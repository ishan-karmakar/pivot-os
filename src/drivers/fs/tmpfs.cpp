#include <drivers/fs/tmpfs.hpp>
#include <sys/stat.h>
using namespace tmpfs;

tmpfs::fs_t::fs_t() : vfs::fs_t{"tmpfs"} {}

vfs::inode_t *fs_t::mount(frg::string_view path) {
    auto *n = new inode_t{path, S_IFDIR};
    n->is_mount = true;
    return n;
}

void tmpfs::init() {
    new fs_t;
}
