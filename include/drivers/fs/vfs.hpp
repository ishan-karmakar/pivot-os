#pragma once
#include <frg/string.hpp>
#include <frg/hash_map.hpp>
#include <mem/heap.hpp>
#include <sys/types.h>
#include <sys/stat.h>

namespace vfs {
    struct dentry_dir_t;
    struct dentry_t {
        dentry_t(dentry_dir_t *parent, std::string_view name, uint32_t mode) : name{name}, parent{parent}, mode{mode} {}
        ~dentry_t();
        dentry_t *follow();

        std::string name;
        dentry_dir_t *parent;
        uint32_t mode;
    };

    struct dentry_file_t : public dentry_t {
        dentry_file_t(dentry_dir_t *parent, std::string_view name, uint32_t mode) : dentry_t{parent, name, mode | S_IFREG} {}
        virtual ~dentry_file_t() = default;
        virtual ssize_t write(void*, std::size_t, off_t) = 0;
        virtual ssize_t read(void*, std::size_t, off_t) = 0;
        virtual void remove() { delete this; }

        std::size_t fsize;
    };
    
    struct dentry_dir_t : public dentry_t {
        dentry_dir_t(dentry_dir_t *parent, std::string_view name, uint32_t mode) : dentry_t{parent, name, mode | S_IFDIR} {}
        virtual ~dentry_dir_t() = default;
        virtual dentry_t *find_child(std::string_view) = 0;
        virtual dentry_t *create_child(std::string_view, uint32_t) = 0;
        // Remove itself - Not a destructor so it can choose to disable deletion (devices)
        virtual void remove() { delete this; }
        virtual void unmount() = 0;
        
        dentry_dir_t *mountp; // nullptr for non mounts, otherwise dentry of mounted root
        std::vector<dentry_t*> children;
    };

    struct dentry_lnk_t : public dentry_t {
        dentry_lnk_t(dentry_dir_t *parent, std::string_view name, uint32_t mode) : dentry_t{parent, name, mode | S_IFLNK} {}
        virtual ~dentry_lnk_t() = default;
        virtual void remove() { delete this; }

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
        virtual vfs::dentry_dir_t *mount(std::string_view) = 0;
    };

    void init();
    std::pair<dentry_dir_t*, dentry_t*> path2ent(std::string_view, bool);
    std::pair<dentry_dir_t*, dentry_t*> path2ent(dentry_dir_t*, std::string_view, bool);
}
