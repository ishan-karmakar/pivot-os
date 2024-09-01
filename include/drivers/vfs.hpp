#pragma once
#include <frg/string.hpp>
#include <unordered_set>

namespace vfs {
    class fs_t;
    struct inode_t {
        inode_t(frg::string_view n) : name{n} {}
        virtual ~inode_t() = default;

        // Creates a inode that is child of current inode
        virtual inode_t *create(frg::string_view) = 0;

        // Unmounts the current inode (represents root of filesystem)
        virtual void unmount() = 0;
    
        frg::string_view name;
        inode_t *parent;
        std::unordered_set<inode_t*> children;
        uint32_t flags;
    };

    class fs_t {
    protected:
        fs_t(frg::string_view);
        virtual ~fs_t() = default;

    public:
        virtual inode_t *mount(frg::string_view) = 0;
    };

    void mount(frg::string_view, frg::string_view);
    void unmount(frg::string_view);
    void remove(frg::string_view, bool);
    void remove(inode_t*, bool);
    void create(frg::string_view, uint32_t flags);
}