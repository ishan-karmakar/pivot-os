#include <drivers/tmpfs.hpp>
using namespace tmpfs;

tmpfs::tmpfs_t::tmpfs_t() : vfs::fs_t{"tmpfs"} {}

vfs::inode_t *tmpfs::tmpfs_t::mount(frg::string_view path) {
    auto n = new vfs::inode_t{path};
    n->fs = this;
    return n;
}

void tmpfs::tmpfs_t::unmount(vfs::inode_t*) {}

void tmpfs::init() {
    new tmpfs_t;
}