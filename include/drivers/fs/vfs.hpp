#pragma once
#include <frg/string.hpp>
#include <unordered_set>

namespace vfs {
    struct cdev_t {
        virtual void write(const void *buffer, std::size_t off, std::size_t count) = 0;
        virtual void read(void *buffer, std::size_t off, std::size_t count) = 0;
    };

    class fs_t;
    struct inode_t {
        inode_t(frg::string_view n, uint32_t flags) : name{n}, flags{flags} {}
        virtual ~inode_t() = default;

        // Unmounts the current inode (represents root of filesystem)
        virtual void unmount() = 0;
    
        frg::string_view name;
        frg::hash_map<frg::string_view, inode_t*, frg::hash<frg::string_view>, heap::allocator> children{{}};
        uint32_t flags;
        bool is_mount{false}; // Figure out where this can go in flags
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