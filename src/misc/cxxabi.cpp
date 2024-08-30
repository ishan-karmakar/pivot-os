#include <cstddef>
#include <cstdint>
#include <misc/cxxabi.hpp>
#include <lib/logger.hpp>
#include <cstdlib>
#include <cxxabi.h>
#include <bits/stl_tree.h>
#include <bits/hashtable_policy.h>
#include <list>
typedef void (*func_t)(void);

extern "C" void (*__init_array_start[])();
extern "C" void (*__init_array_end[])();

void cxxabi::call_constructors() {
    for (auto ctor = __init_array_start; ctor < __init_array_end; ctor++)
        (*ctor)();
    logger::info("CTORS", "Finished running global constructors");
}

namespace std {
    void __throw_bad_function_call() {
        logger::panic("STD", "__throw_bad_function_call()");
    }

    void __throw_length_error(const char *s) {
        logger::panic("STD", "__throw_length_error(), %s", s);
    }

    void __throw_bad_array_new_length() {
        logger::panic("STD", "__throw_bad_array_new_length()");
    }

    void __throw_bad_alloc() {
        logger::panic("STD", "__throw_bad_alloc()");
    }

    [[noreturn]]
    void __throw_system_error(int e) {
        logger::panic("CXXABI", "System error (%d)", e);
    }

    namespace __detail {
        void
        _List_node_base::
        _M_hook(_List_node_base* const __position) noexcept
        {
            this->_M_next = __position;
            this->_M_prev = __position->_M_prev;
            __position->_M_prev->_M_next = this;
            __position->_M_prev = this;
        }

        void
        _List_node_base::_M_unhook() noexcept
        {
            _List_node_base* const __next_node = this->_M_next;
            _List_node_base* const __prev_node = this->_M_prev;
            __prev_node->_M_next = __next_node;
            __next_node->_M_prev = __prev_node;
        }

        void
        _List_node_base::
        _M_transfer(_List_node_base * const __first,
            _List_node_base * const __last) _GLIBCXX_USE_NOEXCEPT
        {
            __glibcxx_assert(__first != __last);

            if (this != __last)
            {
            // Remove [first, last) from its old position.
            __last->_M_prev->_M_next  = this;
            __first->_M_prev->_M_next = __last;
            this->_M_prev->_M_next    = __first;

            // Splice [first, last) into its new position.
            _List_node_base* const __tmp = this->_M_prev;
            this->_M_prev                = __last->_M_prev;
            __last->_M_prev              = __first->_M_prev;
            __first->_M_prev             = __tmp;
            }
        }
    }
}

extern "C" {
    namespace __cxxabiv1 {
        int __cxa_guard_acquire(__guard *guard) {
            if (*guard & 1)
                return 0;
            else if (*guard & 0x100)
                abort();
            
            *guard |= 0x100;
            return 1;
        }

        void __cxa_guard_release(__guard *guard) {
            *guard |= 1;
        }
    }

    void *__dso_handle = nullptr;
    // TODO: Make this actually do something
    int __cxa_atexit(void (*)(void*), void*, void*) {
        return 0;
    }

    int __popcountdi2(int64_t a) {
        uint64_t x2 = (uint64_t)a;
        x2 = x2 - ((x2 >> 1) & 0x5555555555555555uLL);
        // Every 2 bits holds the sum of every pair of bits (32)
        x2 = ((x2 >> 2) & 0x3333333333333333uLL) + (x2 & 0x3333333333333333uLL);
        // Every 4 bits holds the sum of every 4-set of bits (3 significant bits) (16)
        x2 = (x2 + (x2 >> 4)) & 0x0F0F0F0F0F0F0F0FuLL;
        // Every 8 bits holds the sum of every 8-set of bits (4 significant bits) (8)
        uint32_t x = (uint32_t)(x2 + (x2 >> 32));
        // The lower 32 bits hold four 16 bit sums (5 significant bits).
        //   Upper 32 bits are garbage
        x = x + (x >> 16);
        // The lower 16 bits hold two 32 bit sums (6 significant bits).
        //   Upper 16 bits are garbage
        return (x + (x >> 8)) & 0x0000007F; // (7 significant bits)
    }
}
