#include <drivers/fs/tmpfs.hpp>
#include <sys/stat.h>
using namespace tmpfs;

tmpfs::fs_t::fs_t() : vfs::fs_t{"tmpfs"} {}

vfs::dentry_t *fs_t::mount(vfs::dentry_t *parent, frg::string_view name) {
    auto *n = new inode_t;
    n->is_mount = true;
    return new vfs::dentry_t{name, parent, S_IFDIR, n};
}

void tmpfs::init() {
    new fs_t;
}
