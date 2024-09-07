#include <drivers/fs/vfs.hpp>
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

void vfs::mount(frg::string_view path, frg::string_view fs_name) {
    if (filesystems.find(fs_name) == filesystems.end())
        return logger::warning("VFS", "'%s' not found", fs_name.data());
}
