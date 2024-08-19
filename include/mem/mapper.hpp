#pragma once
#include <cstdint>
#include <cstddef>
#include <frg/manual_box.hpp>
#include <array>

namespace mapper {
    typedef uint64_t* page_table;

    enum PagingFlags {
        Writable = (1 << 1),
        User = (1 << 2),
        WriteThrough = (1 << 3),
        CacheDisable = (1 << 4),
        NoExecute = (1UL << 63),
    };

    constexpr std::size_t KERNEL_ENTRY = NoExecute | Writable;
    constexpr std::size_t USER_ENTRY = Writable | User;

    class ptmapper {
    public:
        ptmapper(page_table);
        void map(const uintptr_t&, const uintptr_t&, const std::size_t&);
        void map(const uintptr_t&, const uintptr_t&, const std::size_t&, const std::size_t&);
        uintptr_t translate(const uintptr_t&) const;
        void unmap(const uintptr_t&);
        void unmap(const uintptr_t&, const std::size_t&);
        void load() const;

    private:
        static void clean_table(page_table);
        uintptr_t alloc_table();
        static std::array<uint16_t, 4> get_entries(const uintptr_t&);

        page_table pml4;
    };

    void init();

    extern frg::manual_box<ptmapper> kmapper;
}