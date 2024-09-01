#include <drivers/vfs.hpp>
#include <frg/hash_map.hpp>
#include <mem/heap.hpp>
#include <lib/logger.hpp>
#include <cwalk.h>
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

void vfs::mount(frg::string_view path, frg::string_view name) {
    if (filesystems.find(name) == filesystems.end())
        logger::panic("VFS", "Filesystem '%s' isn't registered", name.data());
    if (path != "/" && !root)
        logger::panic("VFS", "Cannot mount filesystem at non root path without root initialized");
    logger::verbose("VFS", "Mounting filesystem '%s' at '%s'", name.data(), path.data());
    const char *bn;
    cwk_path_get_basename(path.data(), &bn, nullptr);
    logger::info("VFS", "Basename: %s", bn);
    // auto n = filesystems[name]->mount(path);
}