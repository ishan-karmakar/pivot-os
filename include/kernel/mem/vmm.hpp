#pragma once
#include <cstddef>

namespace mem {
    namespace vmm {
        enum vmm_level {
            Supervisor,
            User
        };
    }

    class VirtualMemoryManager {
    public:
        void init(enum vmm::vmm_level, size_t);
    };
}