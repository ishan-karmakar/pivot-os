#pragma once
#include <drivers/fs/vfs.hpp>

namespace devtmpfs {
    struct dentry_file_t : public vfs::dentry_file_t {
        using vfs::dentry_file_t::dentry_file_t;
        ~dentry_file_t() = default;
        ssize_t write(void*, std::size_t, off_t) override;
        ssize_t read(void*, std::size_t, off_t) override;
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
        vfs::dentry_dir_t *mount(vfs::dentry_dir_t*, std::string_view) override;
    };

    void init();
    void register_dev(std::string_view, vfs::cdev_t*);
}