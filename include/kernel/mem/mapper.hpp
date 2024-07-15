#pragma once
#include <cstdint>
#include <cstddef>
#include <mem/pmm.hpp>

namespace mem {
    class PTMapper {
    public:
        PTMapper(pg_tbl_t, PMM&);
        void map(uintptr_t, uintptr_t, size_t);
        void map(uintptr_t, uintptr_t, size_t, size_t);
        uintptr_t translate(uintptr_t);
        void unmap(uintptr_t);
        void unmap(uintptr_t, size_t);
        void load();

    private:
        static void clean_table(pg_tbl_t);

        pg_tbl_t pml4;
        PMM& pmm;
    };
}