#pragma once
#include <vector>
#include <drivers/fs/vfs.hpp>

namespace term {
    void init();
    void register_devs();
    inline void clear();
}