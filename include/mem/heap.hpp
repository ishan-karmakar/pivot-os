#pragma once
#include <frg/slab.hpp>
#include <frg/spinlock.hpp>
#include <mem/vmm.hpp>

namespace heap {
    struct policy {
        policy(vmm::vmm& vmm) : vmm{vmm} {};
        uintptr_t map(std::size_t);
        void unmap(uintptr_t, std::size_t);
    
    private:
        vmm::vmm& vmm;
    };

    typedef frg::slab_pool<policy, frg::simple_spinlock> pool_t;
    typedef frg::slab_allocator<policy, frg::simple_spinlock> allocator_t;

    void init();
    allocator_t& allocator();
}