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

        node *new_node();

        frg::rbtree<node, &node::hook, VMMComparator> tree;
        struct {
            uint32_t num_nodes;
            uint32_t num_pages;
            node nodes[];
        } *nodes;
        size_t flags;
        PTMapper &mapper;

        static constexpr int NODES_PER_PAGE = (PAGE_SIZE - sizeof(uint64_t)) / sizeof(node);
    };

    extern frg::manual_box<VMM> kvmm;
}