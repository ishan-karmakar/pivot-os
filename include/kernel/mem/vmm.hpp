#pragma once
#include <cstddef>
#include <mem/bitmap.hpp>
#include <mem/pmm.hpp>
#include <mem/mapper.hpp>

namespace mem {
    namespace vmm {
        enum vmm_level {
            Supervisor,
            User
        };
    }

    class VirtualMemoryManager : public Bitmap {
    public:
        void init(enum vmm::vmm_level, size_t, PTMapper*, PhysicalMemoryManager*);
        void post_alloc(void*, size_t);
        void post_free(void*, size_t);

    private:
        size_t flags;
        PhysicalMemoryManager *pmm;
        PTMapper *mapper;
    };
}