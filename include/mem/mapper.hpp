#pragma once
#include <cstdint>
#include <cstddef>
#include <frg/manual_box.hpp>

typedef uint64_t* pg_tbl_t;

namespace mapper {
    enum PagingFlags {
        Writable = (1 << 1),
        User = (1 << 2),
        WriteThrough = (1 << 3),
        CacheDisable = (1 << 4),
        Hugepage = (1 << 7),
        NoExecute = (1 << 63)
    };

    constexpr size_t KERNEL_ENTRY = Writable;
    constexpr size_t USER_ENTRY = Writable | User;

    class PTMapper {
    public:
        PTMapper(pg_tbl_t);
        void map(uintptr_t, uintptr_t, size_t);
        void map(uintptr_t, uintptr_t, size_t, size_t);
        uintptr_t translate(uintptr_t) const;
        void unmap(uintptr_t) const;
        void unmap(uintptr_t, size_t) const;
        void load() const;

    private:
        static void clean_table(pg_tbl_t);
        uintptr_t alloc_table();

        pg_tbl_t const pml4;
    };

    void init();

    extern frg::manual_box<PTMapper> kmapper;
}