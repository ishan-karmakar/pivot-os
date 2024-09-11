#pragma once
#include <drivers/fs/vfs.hpp>
#include <sys/stat.h>

namespace devtmpfs {
    struct cdev_t {
        cdev_t() = default;
        virtual ~cdev_t() = default;
        virtual ssize_t write(void*, std::size_t, off_t) = 0;
        virtual ssize_t read(void*, std::size_t, off_t) = 0;
    };

    struct dentry_dir_t : public vfs::dentry_dir_t {
        using vfs::dentry_dir_t::dentry_dir_t;
        ~dentry_dir_t() = default;
        vfs::dentry_t *find_child(std::string_view) override;
        vfs::dentry_t *create_child(std::string_view, uint32_t) override;
        void unmount() override;
    };

    struct dentry_file_t : public vfs::dentry_file_t {
        dentry_file_t(dentry_dir_t*, std::string_view, cdev_t*, mode_t);
        ssize_t write(void*, std::size_t, off_t) override;
        ssize_t read(void*, std::size_t, off_t) override;
        void remove() override;
        ~dentry_file_t() = default;

        cdev_t *dev;
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
    void add_dev(std::string_view, cdev_t*, mode_t);
}
