#include <drivers/tmpfs.hpp>
using namespace tmpfs;

tmpfs::tmpfs::tmpfs() : vfs::fs{"tmpfs"} {}

vfs::fs_instance *tmpfs::tmpfs::mount() {
    return nullptr;
}

void tmpfs::init() {
    new tmpfs;
}