#pragma once
#include <cstdint>
#include <cstddef>
#include <frg/manual_box.hpp>
#include <array>

typedef uint64_t* pg_tbl_t;

namespace mapper {
    enum PagingFlags {
        Writable = (1 << 1),
        User = (1 << 2),
        WriteThrough = (1 << 3),
        CacheDisable = (1 << 4),
        NoExecute = (1UL << 63),
    };

    constexpr std::size_t KERNEL_ENTRY = NoExecute | Writable;
    constexpr std::size_t USER_ENTRY = Writable | User;

    class PTMapper {
    public:
        PTMapper(pg_tbl_t);
        void map(const uintptr_t&, const uintptr_t&, const std::size_t&);
        void map(const uintptr_t&, const uintptr_t&, const std::size_t&, const std::size_t&);
        uintptr_t translate(const uintptr_t&) const;
        void unmap(const uintptr_t&);
        void unmap(const uintptr_t&, const std::size_t&);
        void load() const;

    private:
        static void clean_table(pg_tbl_t);
        uintptr_t alloc_table();
        static std::array<uint16_t, 4> get_entries(const uintptr_t&);

        pg_tbl_t pml4;
    };

    void init();

    extern frg::manual_box<PTMapper> kmapper;
}