#pragma once
#include <frg/slab.hpp>
#include <frg/spinlock.hpp>
#include <mem/vmm.hpp>

namespace heap {
    struct policy_t {
        static constexpr std::size_t sb_size = 64 * PAGE_SIZE;
        static constexpr std::size_t slabsize = 64 * PAGE_SIZE;

        policy_t(vmm::vmm& vmm) : vmm{vmm} {};
        uintptr_t map(std::size_t);
        void unmap(uintptr_t, std::size_t);
    
    private:
        vmm::vmm& vmm;
    };

    typedef frg::slab_pool<policy_t, frg::simple_spinlock> pool_t;
    extern frg::manual_box<policy_t> policy;
    extern frg::manual_box<pool_t> pool;

    struct allocator : public frg::slab_allocator<policy_t, frg::simple_spinlock> {
        allocator();
    };

    void init();
}