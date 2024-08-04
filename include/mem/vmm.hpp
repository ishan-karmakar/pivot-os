#pragma once
#include <cstddef>
#include <frg/manual_box.hpp>
#include <kernel.hpp>
#include <buddy_alloc.h>

namespace mem {
    class PTMapper;

    namespace vmm {
        void init();
    }

    class VMM {
    public:
        enum vmm_level {
            Supervisor,
            User
        };

        VMM(uintptr_t start, size_t size, size_t flags, PTMapper&);
        void *malloc(size_t);
        void free(void*);

    private:
        size_t flags;
        PTMapper &mapper;
        struct buddy *buddy;
    };

    extern frg::manual_box<VMM> kvmm;
}