#include <drivers/vfs.hpp>
#include <frg/hash_map.hpp>
#include <mem/heap.hpp>
#include <lib/logger.hpp>
using namespace vfs;

frg::hash_map<frg::string_view, fs*, frg::hash<frg::string_view>, heap::allocator> filesystems{{}};

fs::fs(frg::string_view name) {
    if (filesystems.find(name) != filesystems.end())
        logger::warning("VFS", "Filesystem '%s' is already registered");
    else
        logger::info("VFS", "Registering new filesystem '%s'", name);
    filesystems[name] = this;
}

void vfs::mount(frg::string_view path, frg::string_view name) {
    if (filesystems.find(name) == filesystems.end())
        return logger::warning("VFS", "Filesystem '%s' isn't registered", name.data());
    logger::verbose("VFS", "Mounting filesystem '%s' at '%s'", name.data(), path.data());
    filesystems[name]->mount();
}