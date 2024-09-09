#include <drivers/fs/devtmpfs.hpp>
#include <sys/stat.h>
#include <lib/logger.hpp>
#include <unordered_map>
using namespace devtmpfs;

std::unordered_map<std::string_view, vfs::cdev_t*> devs;

vfs::dentry_dir_t *fs_t::mount(vfs::dentry_dir_t *node, std::string_view name) {
    auto n = new dentry_dir_t{nullptr, name, S_IFDIR};
    if (node) node->mountp = n;
    return n;
}

vfs::dentry_t *dentry_dir_t::find_child(std::string_view name) {
    for (auto child : children)
        if (child->name == name) return child;
    return nullptr;
}

vfs::dentry_t *dentry_dir_t::create_child(std::string_view name, uint32_t mode) {
    dentry_t *n;
    if (S_ISLNK(mode))
        n = new dentry_lnk_t{this, name, mode};
    else if (S_ISDIR(mode))
        n = new dentry_dir_t{this, name, mode};
    else if (S_ISREG(mode))
        n = new dentry_file_t{this, name, mode};
    else return nullptr;
    children.push_back(n);
    return n;
}

void dentry_dir_t::unmount() {}

ssize_t dentry_file_t::write(void *buffer, std::size_t size, off_t off) {
    if (devs.find(name) == devs.end()) {
        errno = ENODEV;
        return -1;
    }
    return devs[name]->write(buffer, size, off);
}

ssize_t dentry_file_t::read(void *buffer, std::size_t size, off_t off) {
    if (devs.find(name) == devs.end()) {
        errno = ENODEV;
        return -1;
    }
    return devs[name]->read(buffer, size, off);
}

void dentry_file_t::remove() {}

void devtmpfs::init() {
    new fs_t;
}

void devtmpfs::register_dev(std::string_view name, vfs::cdev_t *dev) {
    devs[name] = dev;
}