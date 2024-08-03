#pragma once
#include <cstddef>
#include <frg/manual_box.hpp>
#include <frg/rbtree.hpp>
#include <kernel.hpp>

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

        VMM(uintptr_t start, size_t flags, PTMapper&);
        void *malloc(size_t);
        size_t free(void*);

    private:
        struct node {
            frg::rbtree_hook hook;
            uintptr_t base;
            size_t length;
        };

        struct VMMComparator {
            bool operator()(node& left, node& right) { return left.base < right.base; }
        };

        frg::rbtree<node, &node::hook, VMMComparator> tree;
        size_t flags;
        PTMapper &mapper;
    };

    extern frg::manual_box<VMM> kvmm;
}