#pragma once
#include <frg/string.hpp>
#include <string>

namespace vfs {
    struct dentry {};

    struct inode {};

    struct fs_instance {
    };

    class fs {
    protected:
        fs(frg::string_view);
        virtual ~fs() = default;

    public:
        virtual fs_instance *mount() = 0;
    };

    void mount(frg::string_view, frg::string_view);
}