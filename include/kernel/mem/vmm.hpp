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
        VirtualMemoryManager(enum vmm::vmm_level, size_t, PTMapper&, PhysicalMemoryManager&);

    private:
        void post_alloc(void*, size_t) override;
        void post_free(void*, size_t) override;
        uintptr_t parse_level(enum vmm::vmm_level);

        size_t flags;
        PhysicalMemoryManager& pmm;
        PTMapper& mapper;
    };
}