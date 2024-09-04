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
