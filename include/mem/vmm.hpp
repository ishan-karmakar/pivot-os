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

        VMM(enum vmm_level, PTMapper&);
        void *malloc(size_t);
        size_t free(void*);

    private:
        struct node {
            frg::rbtree_hook hook;
            uintptr_t base;
            size_t size;
        };

        struct VMMComparator {
            bool operator()(node *left, node *right) {
                return left->base <= right->base;
            }
        };

        node *find_inactive(node*);

        size_t flags;
        PTMapper& mapper;
        frg::rbtree<node, &node::hook, VMMComparator> tree;
        static constexpr int NODES_PER_PAGE = PAGE_SIZE / sizeof(node);
    };

    extern frg::manual_box<VMM> kvmm;
}