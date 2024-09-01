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

    class vmm_t {
    public:
        vmm_t(uintptr_t start, std::size_t size, std::size_t flags, mapper::ptmapper&);
        ~vmm_t();
        void *malloc(std::size_t);
        void free(void*);
        mapper::ptmapper& mapper() const { return mpr; }

    private:
        std::size_t flags;
        mapper::ptmapper &mpr;
        struct buddy *buddy;
    };

    extern frg::manual_box<vmm_t> kvmm;
}