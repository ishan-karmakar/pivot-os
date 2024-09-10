#include <drivers/fs/devtmpfs.hpp>
#include <sys/stat.h>
#include <lib/logger.hpp>
using namespace devtmpfs;

std::vector<cdev_t*> devs;

vfs::dentry_dir_t *fs_t::mount(std::string_view name) {
    return new dentry_dir_t{nullptr, name, 0};
}

vfs::dentry_t *dentry_dir_t::find_child(std::string_view name) {
    for (auto child : devs)
        if (child->name == name) return child;
    return nullptr;
}

// Right now we don't allow creation of devices through regular open() or creat() methods
// I don't know how linux does it
vfs::dentry_t *dentry_dir_t::create_child(std::string_view, uint32_t) {
    return nullptr;
}

void dentry_dir_t::unmount() {}

void cdev_t::remove() {}

void devtmpfs::init() {
    new fs_t;
}

void devtmpfs::register_dev(cdev_t *dev) {
    logger::verbose("DEVTMPFS", "Registering device '%s'", dev->name.data());
    devs.push_back(dev);
}
