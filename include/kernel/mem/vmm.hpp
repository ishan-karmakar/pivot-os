#pragma once
#include <cstddef>
#include <mem/bitmap.hpp>
#include <mem/pmm.hpp>
#include <mem/mapper.hpp>

namespace mem {
    class VMM : public Bitmap {
    public:
        enum vmm_level {
            Supervisor,
            User
        };

        VMM(enum vmm_level, size_t, PTMapper&);
        void *alloc(size_t) override;
        size_t free(void*) override;

    private:
        uint8_t * map_bm(enum vmm_level, size_t, PTMapper&);

        size_t flags;
        PTMapper& mapper;
    };
}