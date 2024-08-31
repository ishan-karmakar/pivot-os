#pragma once
#include <drivers/vfs.hpp>

namespace tmpfs {
    class tmpfs : public vfs::fs {
    public:
        tmpfs();
    
    private:
        vfs::fs_instance *mount();
    };

    void init();
}