#pragma once
#include <drivers/vfs.hpp>

namespace tmpfs {
    struct tmpfs_inode_t : public vfs::inode_t {
        tmpfs_inode_t(frg::string_view name) : vfs::inode_t{name} {}
        tmpfs_inode_t *create(frg::string_view) override;
        void unmount() override {}
    };

    class tmpfs_t : public vfs::fs_t {
    public:
        tmpfs_t();

    private:
        vfs::inode_t *mount(frg::string_view) override;
    };

    void init();
}