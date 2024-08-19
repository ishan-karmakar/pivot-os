#pragma once
#include <cstddef>
#include <frg/manual_box.hpp>
#include <kernel.hpp>
#include <buddy_alloc.h>

namespace mapper {
    class ptmapper;
}

namespace vmm {
    void init();

    class vmm {
    public:
        vmm(uintptr_t start, std::size_t size, std::size_t flags, mapper::ptmapper&);
        void *malloc(std::size_t);
        void free(void*);

    private:
        std::size_t flags;
        mapper::ptmapper &mpr;
        struct buddy *buddy;
    };

    extern frg::manual_box<vmm> kvmm;
}