#pragma once
#include <cstddef>
#include <mem/bitmap.hpp>
#include <frg/manual_box.hpp>

namespace mem {
    class PTMapper;

    class VMM : public Bitmap {
    public:
        enum vmm_level {
            Supervisor,
            User
        };

        VMM(enum vmm_level, size_t, PTMapper&);
        void *malloc(size_t) override;
        size_t free(void*) override;

    private:
        uint8_t *map_bm(enum vmm_level, size_t, PTMapper&);

        size_t flags;
        PTMapper& mapper;
    };

    namespace vmm {
        void init();
    }

    extern frg::manual_box<VMM> kvmm;
}