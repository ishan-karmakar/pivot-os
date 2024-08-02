#pragma once
#include <cstdint>
#include <cstddef>
#include <frg/manual_box.hpp>

#define KERNEL_PT_ENTRY 0b10
#define USER_PT_ENTRY 0b110

namespace mem {
    typedef uint64_t* pg_tbl_t;

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

        pg_tbl_t const pml4;
    };

    namespace mapper {
        void init();
    }

    extern frg::manual_box<PTMapper> kmapper;
}