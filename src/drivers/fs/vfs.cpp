#include <drivers/fs/vfs.hpp>
#include <frg/hash_map.hpp>
#include <mem/heap.hpp>
#include <lib/logger.hpp>
#include <cwalk.h>
#include <sys/stat.h>
#include <assert.h>
using namespace vfs;

frg::hash_map<frg::string_view, fs_t*, frg::hash<frg::string_view>, heap::allocator> filesystems{{}};
dentry_t *root;

fs_t::fs_t(frg::string_view name) {
    if (filesystems.find(name) != filesystems.end())
        logger::warning("VFS", "Filesystem '%s' is already registered");
    else
        logger::info("VFS", "Registering new filesystem '%s'", name);
    filesystems[name] = this;
}

dentry_t *path2ent(frg::string_view path) {
    cwk_segment seg;
    if (!cwk_path_get_first_segment(path.data(), &seg)) return root;
    dentry_t *parent = root;
    do {
        frg::string_view name{seg.begin, seg.size};
        auto type = cwk_path_get_segment_type(&seg);
        if (type == CWK_CURRENT) continue;
        else if (type == CWK_BACK) {
            parent = parent->parent; // TODO: Handle if root
            continue;
        } else if (parent->children.find(name) == parent->children.end()) {
            errno = ENOENT;
            return nullptr;
        } else {
            parent = parent->children[name];
            if (S_ISLNK(parent->flags))
                logger::panic("VFS", "Symlinks are unimplemented");
        }
    } while (cwk_path_get_next_segment(&seg));
    return parent;
}

void vfs::mount(frg::string_view path, frg::string_view fs_name) {
    if (filesystems.find(fs_name) == filesystems.end())
        return logger::warning("VFS", "'%s' not found", fs_name.data());
    if (path == "/") {
        if (root)
            logger::warning("VFS", "Root is already initialized, overwriting");
        root = filesystems[fs_name]->mount(nullptr, "/");
        logger::verbose("VFS", "Mounted root filesystem");
    } else {
        if (!root)
            return logger::error("VFS", "Cannot mount filesystem at non root path if root doesn't exist");
    }
}
