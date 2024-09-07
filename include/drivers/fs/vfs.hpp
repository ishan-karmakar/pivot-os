#pragma once
#include <frg/string.hpp>
#include <frg/hash_map.hpp>
#include <mem/heap.hpp>
#include <unordered_set>

namespace vfs {
    struct cdev_t {
        virtual ~cdev_t() = default;
        virtual void write(const void *buffer, std::size_t off, std::size_t count) = 0;
        virtual void read(void *buffer, std::size_t off, std::size_t count) = 0;
    };

    class fs_t;
    struct inode_t {
        virtual ~inode_t() = default;

        // Unmounts the current inode (represents root of filesystem)
        virtual void unmount() = 0;
    
        bool is_mount{false}; // Figure out where this can go in flags
    };

    struct dentry_t {
        dentry_t(frg::string_view name, dentry_t *parent, uint32_t flags, inode_t *inode) : name{name}, parent{parent}, flags{flags}, inode{inode} {}

        frg::string_view name;
        dentry_t *parent;
        uint32_t flags;
        // Basically no space taken up because it is initialized empty
        frg::hash_map<frg::string_view, dentry_t*, frg::hash<frg::string_view>, heap::allocator> children{{}};
        inode_t *inode;
    };

    class fs_t {
    protected:
        fs_t(frg::string_view);
        virtual ~fs_t() = default;

    public:
        virtual dentry_t *mount(dentry_t*, frg::string_view) = 0;
    };

    void mount(frg::string_view, frg::string_view);
    void unmount(frg::string_view);
    void remove(frg::string_view, bool);
    void remove(inode_t*, bool);
    void create(frg::string_view, uint32_t flags);
}