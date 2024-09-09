#pragma once
#include <frg/string.hpp>
#include <frg/hash_map.hpp>
#include <mem/heap.hpp>
#include <unordered_set>
#include <sys/types.h>
#include <variant>

namespace vfs {
    struct cdev_t {
        virtual ~cdev_t() = default;
        virtual void write(const void *buffer, std::size_t off, std::size_t count) = 0;
        virtual void read(void *buffer, std::size_t off, std::size_t count) = 0;
    };

    struct dentry_dir_t;
    struct dentry_t {
        dentry_t(dentry_dir_t *parent, std::string_view name, uint32_t mode) : name{name}, parent{parent}, mode{mode} {}
        ~dentry_t();
        dentry_t *follow();

        std::string_view name;
        dentry_dir_t *parent;
        uint32_t mode;
    };

    struct dentry_file_t : public dentry_t {
        using dentry_t::dentry_t;
        virtual ~dentry_file_t() = default;
        virtual ssize_t write(void*, std::size_t, off_t) = 0;
        virtual ssize_t read(void*, std::size_t, off_t) = 0;

        std::size_t fsize;
    };
    
    struct dentry_dir_t : public dentry_t {
        using dentry_t::dentry_t;
        virtual ~dentry_dir_t() = default;
        virtual dentry_t *find_child(std::string_view) = 0;
        virtual dentry_t *create_child(std::string_view, uint32_t) = 0;
        virtual void unmount() = 0;
        
        bool is_mount{false};
        std::vector<dentry_t*> children;
    };

    struct dentry_lnk_t : public dentry_t {
        using dentry_t::dentry_t;
        virtual ~dentry_lnk_t() = default;

        dentry_t *target;
    };

    struct fd_t {
        dentry_t *dentry;
        int flags;
        off_t off;
    };

    class fs_t {
    protected:
        fs_t(std::string_view);
        virtual ~fs_t() = default;

    public:
        virtual dentry_dir_t *mount(dentry_dir_t*, std::string_view) = 0;
    };

    void init();
    void mount(std::string_view, std::string_view);
    void unmount(std::string_view);
}