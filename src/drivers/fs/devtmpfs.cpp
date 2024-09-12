#include <drivers/fs/devtmpfs.hpp>
#include <sys/stat.h>
#include <lib/logger.hpp>
#include <unistd.h>
#include <syscall.h>
using namespace devtmpfs;

std::vector<cdev_t*> devs;
dentry_dir_t *dev_root;

vfs::dentry_dir_t *fs_t::mount(std::string_view name) {
    return dev_root = new dentry_dir_t{nullptr, name, 0};
}

vfs::dentry_t *dentry_dir_t::find_child(std::string_view name) {
    for (auto child : children)
        if (child->name == name) return child;
    return nullptr;
}

// Right now we don't allow creation of devices through regular open() or creat() methods
// I don't know how linux does it
vfs::dentry_t *dentry_dir_t::create_child(std::string_view, uint32_t) {
    return nullptr;
}

void dentry_dir_t::unmount() {}

dentry_file_t::dentry_file_t(dentry_dir_t *parent, std::string_view name, cdev_t *dev, mode_t mode) : vfs::dentry_file_t{parent, name, mode}, dev{dev} {}

ssize_t dentry_file_t::read(void *buf, std::size_t size, off_t off) {
    return dev->read(buf, size, off);
}

ssize_t dentry_file_t::write(void *buf, std::size_t size, off_t off) {
    return dev->write(buf, size, off);
}

void dentry_file_t::remove() {}

struct null_cdev_t : cdev_t {
    ssize_t read(void*, std::size_t, off_t) { return 0; }
    ssize_t write(void*, std::size_t, off_t) { return 0; }
};

struct zero_cdev_t : cdev_t {
    ssize_t read(void *buffer, std::size_t size, off_t) {
        memset(buffer, 0, size);
        return size;
    }

    ssize_t write(void*, std::size_t size, off_t) { return size; }
};

// TODO: random + urandom

void devtmpfs::init() {
    new fs_t;
    syscall(SYS_mkdir, "/dev", S_IRWXU);
    syscall(SYS_mount, "", "/dev", "devtmpfs");
    add_dev("null", new null_cdev_t, S_IFCHR | S_IRWXU);
    add_dev("zero", new zero_cdev_t, S_IFCHR | S_IRWXU);
}

void devtmpfs::add_dev(std::string_view name, cdev_t *dev, mode_t mode) {
    if (!dev_root)
        return logger::error("DEVTMPFS", "Dev root not initialized yet");
    logger::verbose("DEVTMPFS", "Registering device '%s'", name.data());
    auto [parent, _] = vfs::path2ent(dev_root, name, true);
    if (!parent)
	return logger::warning("DEVTMPFS", "'%s' not found", name);
    parent->children.push_back(new dentry_file_t{dev_root, name, dev, mode});
}
