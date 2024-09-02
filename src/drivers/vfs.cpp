#include <drivers/vfs.hpp>
#include <frg/hash_map.hpp>
#include <mem/heap.hpp>
#include <lib/logger.hpp>
#include <cwalk.h>
#include <sys/stat.h>
#include <assert.h>
using namespace vfs;

frg::hash_map<frg::string_view, fs_t*, frg::hash<frg::string_view>, heap::allocator> filesystems{{}};
inode_t *root;

fs_t::fs_t(frg::string_view name) {
    if (filesystems.find(name) != filesystems.end())
        logger::warning("VFS", "Filesystem '%s' is already registered");
    else
        logger::info("VFS", "Registering new filesystem '%s'", name);
    filesystems[name] = this;
}

inode_t *path2node(frg::string_view path) {
    cwk_segment seg;
    inode_t *cur_node = root;
    if (!cwk_path_get_first_segment(path.data(), &seg) || !root)
        return cur_node;
    
    do {
        if (!S_ISDIR(cur_node->flags)) return nullptr;
        for (const auto& child : cur_node->children) {
            if (child->name == frg::string_view{seg.begin, seg.size}) {
                cur_node = child;
                goto next_segment;
            }
        }
        return nullptr;
        next_segment:;
    } while (cwk_path_get_next_segment(&seg));
    return cur_node;
}

void vfs::mount(frg::string_view path, frg::string_view name) {
    if (filesystems.find(name) == filesystems.end())
        logger::panic("VFS", "Filesystem '%s' isn't registered", name.data());
    if (path != "/" && !root)
        logger::panic("VFS", "Cannot mount filesystem at non root path without root initialized");
    logger::verbose("VFS", "Mounting filesystem '%s' at '%s'", name.data(), path.data());
    const char *bn = basename(path.data());
    auto n = filesystems[name]->mount(bn ? bn : "");
    n->flags |= S_IFDIR | 1; // 1 represents this inode is root of a FS
    if (path == "/")
        root = n;
    else {
        std::size_t size;
        cwk_path_get_dirname(path.data(), &size);
        inode_t *parent = path2node(frg::string_view{path.data(), size});
        assert(parent);
        parent->children.insert(n);
        n->parent = parent;
    }
}

void vfs::unmount(frg::string_view path) {
    inode_t *n = path2node(path);
    assert(n && (n->flags & 1));
    logger::verbose("VFS", "Unmounting '%s'", path.data());
    for (const auto& child : n->children)
        remove(child, true);
    n->unmount();
    delete n;
    if (n == root)
        root = nullptr;
    else
        n->parent->children.erase(n);
}

void vfs::create(frg::string_view name, uint32_t flags) {
    std::size_t size;
    cwk_path_get_dirname(name.data(), &size);
    inode_t *parent = path2node(frg::string_view{name.data(), size});
    assert(parent);
    parent->create(basename(name));
}

void vfs::remove(inode_t *node, bool recursive) {
    if (recursive)
        for (const auto& child : node->children)
            remove(child, true);
    delete node;
    node->parent->children.erase(node);
}

void vfs::remove(frg::string_view path, bool recursive) {
    inode_t *n = path2node(path);
    assert(n);
    remove(n, recursive);
}

const char *basename(const char *path) noexcept {
    const char *bn;
    cwk_path_get_basename(path, &bn, nullptr);
    return bn;
}