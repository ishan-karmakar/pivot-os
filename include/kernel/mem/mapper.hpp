#pragma once
#include <cstdint>
#include <cstddef>
#include <mem/pmm.hpp>

namespace mem {
    class PTMapper {
    public:
        PTMapper(pg_tbl_t const);
        void map(uintptr_t, uintptr_t, size_t);
        void map(uintptr_t, uintptr_t, size_t, size_t);
        uintptr_t translate(uintptr_t) const;
        void unmap(uintptr_t) const;
        void unmap(uintptr_t, size_t) const;
        void load() const;

    private:
        static void clean_table(pg_tbl_t);

        pg_tbl_t const pml4;
    };
}