#include <drivers/fs/tmpfs.hpp>
#include <sys/stat.h>
#include <lib/logger.hpp>
using namespace tmpfs;

tmpfs::fs_t::fs_t() : vfs::fs_t{"tmpfs"} {}

vfs::dentry_dir_t *fs_t::mount(vfs::dentry_dir_t *parent, std::string_view name) {
    return new tmpfs::dentry_dir_t{parent, name, S_IFDIR};
}

vfs::dentry_t *dentry_dir_t::find_child(std::string_view name) {
    for (auto child : children)
        if (child->name == name) return child;
    return nullptr;
}

vfs::dentry_t *dentry_dir_t::create_child(std::string_view name, uint32_t mode) {
    dentry_t *n;
    if (S_ISLNK(mode))
        n = new dentry_lnk_t{this, name, mode};
    else if (S_ISDIR(mode))
        n = new dentry_dir_t{this, name, mode};
    else if (S_ISREG(mode))
        n = new dentry_file_t{this, name, mode};
    else return nullptr;
    children.push_back(n);
    return n;
}

ssize_t dentry_file_t::write(void *buffer, std::size_t size, off_t off) {
    if (!start) {
        start = static_cast<char*>(vmm::kvmm->malloc(size));
        buf_size = round_up(size, PAGE_SIZE);
    } else if ((off + size) > buf_size) {
        // TODO: Linux supports seeking to past the end of file, but fills in with zeroes if written to
        // Right now we can do the same but it is garbage. Change to zeroes
        char *new_start = static_cast<char*>(vmm::kvmm->malloc(off + size));
        memcpy(new_start, start, buf_size);
        vmm::kvmm->free(start);
        start = new_start;
        buf_size = off + size;
    }
    memcpy(start + off, buffer, size);
    fsize = std::max(fsize, off + size);
    return size;
}

ssize_t dentry_file_t::read(void *buffer, std::size_t size, off_t off) {
    if (!start)
        return 0;
    
    std::size_t rbytes = std::min(size, fsize - off);
    memcpy(buffer, start + off, rbytes);
    return rbytes;
}

dentry_file_t::~dentry_file_t() {
    vmm::kvmm->free(start);
    // std::erase(parent->children.begin(), parent->children.end(), )
}

void dentry_dir_t::unmount() {}

void tmpfs::init() {
    new fs_t;
}
