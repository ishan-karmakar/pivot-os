#pragma once
#include <drivers/vfs.hpp>

namespace tmpfs {
    class tmpfs_t : public vfs::fs_t {
    public:
        tmpfs_t();

    private:
        vfs::inode_t *mount(frg::string_view) override;
        void unmount(vfs::inode_t*) override;
    };

    void init();
}