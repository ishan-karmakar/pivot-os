#pragma once
#include <drivers/fs/vfs.hpp>
#include <sys/stat.h>

namespace devtmpfs {
    struct cdev_t : public vfs::dentry_file_t {
	cdev_t(std::string_view name) : vfs::dentry_file_t{nullptr, name, 0} {}
	virtual ~cdev_t() = default;
	void remove() override;
    };

    struct dentry_dir_t : public vfs::dentry_dir_t {
        using vfs::dentry_dir_t::dentry_dir_t;
        ~dentry_dir_t() = default;
        vfs::dentry_t *find_child(std::string_view) override;
        vfs::dentry_t *create_child(std::string_view, uint32_t) override;
        void unmount() override;
    };

    struct dentry_lnk_t : public vfs::dentry_lnk_t {
        using vfs::dentry_lnk_t::dentry_lnk_t;
        ~dentry_lnk_t() = default;
    };

    class fs_t : vfs::fs_t {
    public:
        fs_t() : vfs::fs_t{"devtmpfs"} {}
    
    private:
        vfs::dentry_dir_t *mount(std::string_view) override;
    };

    void init();
    void register_dev(cdev_t*);
}
