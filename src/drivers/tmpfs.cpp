#include <drivers/tmpfs.hpp>
using namespace tmpfs;

tmpfs::tmpfs_t::tmpfs_t() : vfs::fs_t{"tmpfs"} {}

vfs::inode_t *tmpfs_t::mount(frg::string_view path) {
    return new tmpfs_inode_t{path};
}

void tmpfs_t::unmount(vfs::inode_t*) {}

void tmpfs::init() {
    new tmpfs_t;
}

tmpfs_inode_t *tmpfs_inode_t::create(frg::string_view name) {
    auto node = new tmpfs_inode_t{name};
    node->parent = this;
    children.push_back(node);
    return node;
}