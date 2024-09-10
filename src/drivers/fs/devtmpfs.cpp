#include <drivers/fs/devtmpfs.hpp>
#include <sys/stat.h>
#include <lib/logger.hpp>
#include <unistd.h>
#include <syscall.h>
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

dentry_file_t::dentry_file_t(dentry_dir_t *parent, std::string_view name, cdev_t *dev) : vfs::dentry_file_t{parent, name, S_IFBLK}, dev{dev} {}

void dentry_file_t::remove() {}

void devtmpfs::init() {
    new fs_t;
    syscall(SYS_mkdir, "/dev", S_IRWXU);
    syscall(SYS_mount, "", "/dev", "devtmpfs");
}

void devtmpfs::register_dev(cdev_t *dev) {
    devs.push_back(dev);
}

void devtmpfs::add_dev(std::string_view name, uint32_t maj, uint32_t min) {
    auto dev = std::find(devs.begin(), devs.end(), std::make_pair(maj, min));
    if (dev == devs.end())
        return logger::warning("DEVTMPFS", "Device not found");
}
