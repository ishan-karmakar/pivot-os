#pragma once
#include <frg/string.hpp>

namespace vfs {
    class fs_t;
    struct inode_t {
        inode_t(frg::string_view n) : name{n} {}

        // Creates a inode that is child of current inode
        virtual inode_t *create(frg::string_view) = 0;
    
        frg::string_view name;
        inode_t *parent;
        std::vector<inode_t*> children;
        uint32_t flags;
    };

    class fs_t {
    protected:
        fs_t(frg::string_view);
        virtual ~fs_t() = default;

    public:
        virtual inode_t *mount(frg::string_view) = 0;
        virtual void unmount(inode_t*) = 0;
    };

    void mount(frg::string_view, frg::string_view);
}