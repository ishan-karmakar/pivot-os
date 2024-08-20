#pragma once
#include <frg/slab.hpp>
#include <frg/spinlock.hpp>
#include <mem/vmm.hpp>

namespace heap {
    struct policy {
        static constexpr std::size_t sb_size = 64 * PAGE_SIZE;
        static constexpr std::size_t slabsize = 64 * PAGE_SIZE;

        policy(vmm::vmm& vmm) : vmm{vmm} {};
        uintptr_t map(std::size_t);
        void unmap(uintptr_t, std::size_t);
    
    private:
        vmm::vmm& vmm;
    };

    typedef frg::slab_pool<policy, frg::simple_spinlock> pool_t;
    struct allocator_t : public frg::slab_allocator<policy, frg::simple_spinlock> {
        allocator_t();
    };
    
    void init();
    allocator_t& allocator();

}