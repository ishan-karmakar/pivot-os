#include <drivers/fs/vfs.hpp>
#include <frg/hash_map.hpp>
#include <mem/heap.hpp>
#include <lib/logger.hpp>
#include <lib/syscalls.hpp>
#include <cwalk.h>
#include <sys/stat.h>
#include <assert.h>
#include <cpu/cpu.hpp>
#include <cpu/smp.hpp>
#include <fcntl.h>
#include <syscall.h>
using namespace vfs;
#define RETURN_ERR(code) { \
    errno = code; \
    return nullptr; \
}

std::unordered_map<std::string_view, fs_t*> filesystems;
dentry_dir_t *root;

fs_t::fs_t(std::string_view name) {
    if (filesystems.find(name) != filesystems.end())
        logger::warning("VFS", "Filesystem '%s' is already registered");
    else
        logger::info("VFS", "Registering new filesystem '%s'", name.data());
    filesystems[name] = this;
}

dentry_t *dentry_t::follow() {
    if (!S_ISLNK(mode)) return this;
    auto cur = this;
    do { cur = static_cast<dentry_lnk_t*>(cur)->target; } while (S_ISLNK(cur->mode));
    return cur;
}

dentry_t::~dentry_t() {
    // FIXME: We shouldn't delete directory when unmount occurs
    parent->children.erase(std::remove(parent->children.begin(), parent->children.end(), this), parent->children.end());
}

std::pair<dentry_dir_t*, dentry_t*> vfs::path2ent(std::string_view path, bool follow) {
    return path2ent(root, path, follow);
}

// TODO: Traverse with permission checking
std::pair<dentry_dir_t*, dentry_t*> vfs::path2ent(dentry_dir_t *root_dent, std::string_view path, bool follow) {
    cwk_segment seg;
    if (!cwk_path_get_first_segment(path.data(), &seg)) return { nullptr, root };
    dentry_t *parent = root_dent;
    do {
        if (!S_ISDIR(parent->mode)) {
            errno = ENOTDIR;
            return { nullptr, nullptr };
        }
        if (!follow)
            parent = parent->follow();
        std::string_view name{seg.begin, seg.size};
        auto type = cwk_path_get_segment_type(&seg);
        if (type == CWK_CURRENT) continue;
        else if (type == CWK_BACK) {
            parent = parent->parent;
            if (!parent) {
                errno = ENOENT;
                return { nullptr, nullptr };
            }
            continue;
        } else {
            auto tmp = static_cast<dentry_dir_t*>(parent)->find_child(name);
            if (!tmp) {
                errno = ENOENT;
                return { static_cast<dentry_dir_t*>(parent), nullptr };
            }
            parent = tmp;
            if (follow)
                parent = parent->follow();
            if (S_ISDIR(parent->mode)) {
                auto d = static_cast<dentry_dir_t*>(parent);
                parent = d->mountp ? d->mountp : d;
            }
        }
    } while (cwk_path_get_next_segment(&seg));
    return { parent->parent, parent };
}

cpu::status *sys_mount(cpu::status *status) {
    status->rax = -1;
    // const char *source = reinterpret_cast<const char*>(status->rsi);
    std::string_view target{reinterpret_cast<const char*>(status->rdx)};
    std::string_view fs_type{reinterpret_cast<const char*>(status->rcx)};
    if (filesystems.find(fs_type) == filesystems.end()) RETURN_ERR(ENODEV)
    if (target == "/")
        root = filesystems[fs_type]->mount("/");
    else {
        if (!root) RETURN_ERR(ENOENT)
        auto [parent, n] = path2ent(target, true);
	if (!n) RETURN_ERR(ENOENT)
        if (!S_ISDIR(n->mode)) RETURN_ERR(ENOTDIR)
	auto d = static_cast<dentry_dir_t*>(n);
        d->mountp = filesystems[fs_type]->mount(target);
	d->mountp->parent = d->parent;
    }
    status->rax = 0;
    return nullptr;
}

cpu::status *sys_open(cpu::status *status) {
    status->rax = -1;
    const char *path = reinterpret_cast<const char*>(status->rsi);
    int flags = status->rdx;
    auto [parent, ent] = path2ent(path, true);
    if (!ent) {
        if (errno != ENOENT)
            return nullptr;
        if (!(flags & O_CREAT))
            RETURN_ERR(ENOENT)
        const char *bn;
        cwk_path_get_basename(path, &bn, nullptr);
        if (!parent) RETURN_ERR(ENOENT)
        ent = parent->create_child(bn, static_cast<mode_t>(status->rcx) | S_IFREG);
    } else if (S_ISDIR(ent->mode)) RETURN_ERR(EISDIR);
    auto& fd_table = smp::this_cpu()->cur_proc->fd_table;
    fd_t fd_ent = { ent, flags, 0 };
    for (std::size_t i = 0; i < fd_table.size(); i++)
        if (!fd_table[i].dentry) {
            status->rax = i;
            fd_table[i] = fd_ent;
            return nullptr;
        }
    status->rax = fd_table.size();
    fd_table.push_back(fd_ent);
    status->rax = 0;
    return nullptr;
}

cpu::status *sys_close(cpu::status *status) {
    status->rax = -1;
    fd_t& ent = smp::this_cpu()->cur_proc->fd_table[status->rsi];
    if (!ent.dentry)
        RETURN_ERR(EBADF)
    ent.dentry = nullptr;
    status->rax = 0;
    return nullptr;
}

cpu::status *sys_write(cpu::status *status) {
    status->rax = -1;
    fd_t& ent = smp::this_cpu()->cur_proc->fd_table[status->rsi];
    void *buffer = reinterpret_cast<void*>(status->rdx);
    std::size_t buf_size = status->rcx;
    uint32_t acc = ent.flags & O_ACCMODE;
    if (!(ent.dentry && S_ISREG(ent.dentry->mode) && (acc == O_RDWR || acc == O_WRONLY)))
        RETURN_ERR(EBADF)
    else if (!buf_size) {
        status->rax = 0;
        return nullptr;
    }
    // We made sure dent is a regular file
    auto f = static_cast<dentry_file_t*>(ent.dentry);
    if (ent.flags & O_APPEND)
        ent.off = f->fsize;

    status->rax = f->write(buffer, buf_size, ent.off);
    ent.off += status->rax;
    status->rax = 0;
    return nullptr;
}

cpu::status *sys_read(cpu::status *status) {
    status->rax = -1;
    fd_t& ent = smp::this_cpu()->cur_proc->fd_table[status->rsi];
    void *buffer = reinterpret_cast<void*>(status->rdx);
    std::size_t rbytes = status->rcx;
    uint32_t acc = ent.dentry->mode & O_ACCMODE;
    if (!(ent.dentry && S_ISREG(ent.dentry->mode) && (acc == O_RDONLY || acc == O_RDONLY)))
        RETURN_ERR(EBADF)
    else if (!rbytes) {
        status->rax = 0;
        return nullptr;
    }

    status->rax = static_cast<dentry_file_t*>(ent.dentry)->read(buffer, rbytes, ent.off);
    ent.off += status->rax;
    return nullptr;
}

cpu::status *sys_lseek(cpu::status *status) {
    status->rax = -1;
    fd_t& ent = smp::this_cpu()->cur_proc->fd_table[status->rsi];
    off_t off = status->rdx;
    int whence = status->rcx;
    if (!(ent.dentry && S_ISREG(ent.dentry->mode)))
        RETURN_ERR(EBADF)

    // We already made sure dent is a regular file
    if (whence == SEEK_SET)
        ent.off = off;
    else if (whence == SEEK_CUR)
        ent.off += whence;
    else if (whence == SEEK_END)
        ent.off = static_cast<dentry_file_t*>(ent.dentry)->fsize + off;

    status->rax = 0;
    return nullptr;
}

cpu::status *sys_mkdir(cpu::status *status) {
    status->rax = -1;
    const char *path = reinterpret_cast<const char*>(status->rsi);
    mode_t mode = status->rdx;
    auto [parent, child] = path2ent(path, true);
    if (child) RETURN_ERR(EEXIST)
    if (!parent) return nullptr;
    const char *bn;
    cwk_path_get_basename(path, &bn, nullptr);
    static_cast<dentry_dir_t*>(parent)->create_child(bn, mode | S_IFDIR);
    status->rax = 0;
    return nullptr;
}

cpu::status *sys_symlink(cpu::status *status) {
    status->rax = -1;
    const char *source = reinterpret_cast<const char*>(status->rsi);
    const char *target = reinterpret_cast<const char*>(status->rdx);
    auto [_, src_ent] = path2ent(source, true);
    if (!src_ent) return nullptr;
    auto [parent, child] = path2ent(target, true);
    if (child) RETURN_ERR(EEXIST)
    const char *bn;
    cwk_path_get_basename(target, &bn, nullptr);
    static_cast<dentry_lnk_t*>(parent->create_child(bn, S_IFLNK))->target = src_ent;
    status->rax = 0;
    return nullptr;
}

cpu::status *sys_rmdir(cpu::status *status) {
    status->rax = -1;
    const char *dir = reinterpret_cast<const char*>(status->rsi);
    auto [parent, child] = path2ent(dir, true);
    if (!child) return nullptr;
    if (!S_ISDIR(child->mode)) RETURN_ERR(ENOTDIR)
    auto d = static_cast<dentry_dir_t*>(child);
    if (!d->children.empty()) RETURN_ERR(ENOTEMPTY)
    d->remove();

    status->rax = 0;
    return nullptr;
}

cpu::status *sys_unlink(cpu::status *status) {
    status->rax = -1;

    const char *target = reinterpret_cast<const char*>(status->rsi);
    auto [parent, child] = path2ent(target, false);
    if (S_ISDIR(child->mode)) RETURN_ERR(EISDIR)
    else if (S_ISREG(child->mode)) static_cast<dentry_file_t*>(child)->remove();
    else if (S_ISLNK(child->mode)) static_cast<dentry_lnk_t*>(child)->remove();

    status->rax = 0;
    return nullptr;
}

void vfs::init() {
    // Register syscalls
    syscalls::handlers[SYS_open] = sys_open;
    syscalls::handlers[SYS_close] = sys_close;
    syscalls::handlers[SYS_write] = sys_write;
    syscalls::handlers[SYS_read] = sys_read;
    syscalls::handlers[SYS_lseek] = sys_lseek;
    syscalls::handlers[SYS_mkdir] = sys_mkdir;
    syscalls::handlers[SYS_symlink] = sys_symlink;
    syscalls::handlers[SYS_unlink] = sys_unlink;
    syscalls::handlers[SYS_rmdir] = sys_rmdir;
    syscalls::handlers[SYS_mount] = sys_mount;
}
