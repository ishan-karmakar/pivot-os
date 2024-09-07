#pragma once
#include <drivers/fs/vfs.hpp>

namespace tmpfs {
    struct inode_t : public vfs::inode_t {
        using vfs::inode_t::inode_t;
        void unmount() override {}
    };

    class fs_t : public vfs::fs_t {
    public:
        fs_t();

    private:
        vfs::inode_t *mount(frg::string_view) override;
    };

    void init();
}