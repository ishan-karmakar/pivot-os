#include <cstddef>
#include <cstdint>
#include <misc/cxxabi.hpp>
#include <lib/logger.hpp>
#include <cstdlib>
#include <cxxabi.h>
#include <bits/stl_tree.h>
#include <bits/hashtable_policy.h>
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

    // Taken from libstdc++, needed for std::deque
    static void local_Rb_tree_rotate_left(_Rb_tree_node_base* const __x,
                        _Rb_tree_node_base*& __root)
    {
        _Rb_tree_node_base* const __y = __x->_M_right;

        __x->_M_right = __y->_M_left;
        if (__y->_M_left !=0)
        __y->_M_left->_M_parent = __x;
        __y->_M_parent = __x->_M_parent;

        if (__x == __root)
        __root = __y;
        else if (__x == __x->_M_parent->_M_left)
        __x->_M_parent->_M_left = __y;
        else
        __x->_M_parent->_M_right = __y;
        __y->_M_left = __x;
        __x->_M_parent = __y;
    }

    // Taken from libstdc++, needed for std::deque
    static void local_Rb_tree_rotate_right(_Rb_tree_node_base* const __x,
                    _Rb_tree_node_base*& __root)
    {
        _Rb_tree_node_base* const __y = __x->_M_left;

        __x->_M_left = __y->_M_right;
        if (__y->_M_right != 0)
        __y->_M_right->_M_parent = __x;
        __y->_M_parent = __x->_M_parent;

        if (__x == __root)
        __root = __y;
        else if (__x == __x->_M_parent->_M_right)
        __x->_M_parent->_M_right = __y;
        else
        __x->_M_parent->_M_left = __y;
        __y->_M_right = __x;
        __x->_M_parent = __y;
    }

    // Taken from libstdc++, needed for std::deque
    void _Rb_tree_insert_and_rebalance(const bool          __insert_left,
                                _Rb_tree_node_base* __x,
                                _Rb_tree_node_base* __p,
                                _Rb_tree_node_base& __header) throw ()
    {
        _Rb_tree_node_base *& __root = __header._M_parent;

        // Initialize fields in new node to insert.
        __x->_M_parent = __p;
        __x->_M_left = 0;
        __x->_M_right = 0;
        __x->_M_color = _S_red;

        // Insert.
        // Make new node child of parent and maintain root, leftmost and
        // rightmost nodes.
        // N.B. First node is always inserted left.
        if (__insert_left)
        {
            __p->_M_left = __x; // also makes leftmost = __x when __p == &__header

            if (__p == &__header)
            {
                __header._M_parent = __x;
                __header._M_right = __x;
            }
            else if (__p == __header._M_left)
            __header._M_left = __x; // maintain leftmost pointing to min node
        }
        else
        {
            __p->_M_right = __x;

            if (__p == __header._M_right)
            __header._M_right = __x; // maintain rightmost pointing to max node
        }
        // Rebalance.
        while (__x != __root
        && __x->_M_parent->_M_color == _S_red)
        {
        _Rb_tree_node_base* const __xpp = __x->_M_parent->_M_parent;

        if (__x->_M_parent == __xpp->_M_left)
        {
            _Rb_tree_node_base* const __y = __xpp->_M_right;
            if (__y && __y->_M_color == _S_red)
            {
            __x->_M_parent->_M_color = _S_black;
            __y->_M_color = _S_black;
            __xpp->_M_color = _S_red;
            __x = __xpp;
            }
            else
            {
            if (__x == __x->_M_parent->_M_right)
            {
                __x = __x->_M_parent;
                local_Rb_tree_rotate_left(__x, __root);
            }
            __x->_M_parent->_M_color = _S_black;
            __xpp->_M_color = _S_red;
            local_Rb_tree_rotate_right(__xpp, __root);
            }
        }
        else
        {
            _Rb_tree_node_base* const __y = __xpp->_M_left;
            if (__y && __y->_M_color == _S_red)
            {
            __x->_M_parent->_M_color = _S_black;
            __y->_M_color = _S_black;
            __xpp->_M_color = _S_red;
            __x = __xpp;
            }
            else
            {
            if (__x == __x->_M_parent->_M_left)
            {
                __x = __x->_M_parent;
                local_Rb_tree_rotate_right(__x, __root);
            }
            __x->_M_parent->_M_color = _S_black;
            __xpp->_M_color = _S_red;
            local_Rb_tree_rotate_left(__xpp, __root);
            }
        }
        }
        __root->_M_color = _S_black;
    }

    static _Rb_tree_node_base* local_Rb_tree_decrement(_Rb_tree_node_base* __x) throw ()
    {
        if (__x->_M_color == _S_red
            && __x->_M_parent->_M_parent == __x)
        __x = __x->_M_right;
        else if (__x->_M_left != 0)
        {
            _Rb_tree_node_base* __y = __x->_M_left;
            while (__y->_M_right != 0)
            __y = __y->_M_right;
            __x = __y;
        }
        else
        {
            _Rb_tree_node_base* __y = __x->_M_parent;
            while (__x == __y->_M_left)
            {
                __x = __y;
                __y = __y->_M_parent;
            }
            __x = __y;
        }
        return __x;
    }

    _Rb_tree_node_base* _Rb_tree_decrement(_Rb_tree_node_base* __x) throw ()
    {
        return local_Rb_tree_decrement(__x);
    }

    static _Rb_tree_node_base* local_Rb_tree_increment(_Rb_tree_node_base* __x) throw ()
    {
        if (__x->_M_right != 0)
        {
            __x = __x->_M_right;
            while (__x->_M_left != 0)
            __x = __x->_M_left;
        }
        else
        {
            _Rb_tree_node_base* __y = __x->_M_parent;
            while (__x == __y->_M_right)
            {
                __x = __y;
                __y = __y->_M_parent;
            }
            if (__x->_M_right != __y)
            __x = __y;
        }
        return __x;
    }

    _Rb_tree_node_base* _Rb_tree_increment(_Rb_tree_node_base* __x) throw ()
    {
        return local_Rb_tree_increment(__x);
    }

    const _Rb_tree_node_base* _Rb_tree_increment(const _Rb_tree_node_base* __x) throw ()
    {
        return local_Rb_tree_increment(const_cast<_Rb_tree_node_base*>(__x));
    }

    _Rb_tree_node_base* _Rb_tree_rebalance_for_erase(_Rb_tree_node_base* const __z,
                    _Rb_tree_node_base& __header) throw ()
    {
        _Rb_tree_node_base *& __root = __header._M_parent;
        _Rb_tree_node_base *& __leftmost = __header._M_left;
        _Rb_tree_node_base *& __rightmost = __header._M_right;
        _Rb_tree_node_base* __y = __z;
        _Rb_tree_node_base* __x = 0;
        _Rb_tree_node_base* __x_parent = 0;

        if (__y->_M_left == 0)     // __z has at most one non-null child. y == z.
        __x = __y->_M_right;     // __x might be null.
        else
        if (__y->_M_right == 0)  // __z has exactly one non-null child. y == z.
        __x = __y->_M_left;    // __x is not null.
        else
        {
        // __z has two non-null children.  Set __y to
        __y = __y->_M_right;   //   __z's successor.  __x might be null.
        while (__y->_M_left != 0)
            __y = __y->_M_left;
        __x = __y->_M_right;
        }
        if (__y != __z)
        {
        // relink y in place of z.  y is z's successor
        __z->_M_left->_M_parent = __y;
        __y->_M_left = __z->_M_left;
        if (__y != __z->_M_right)
        {
            __x_parent = __y->_M_parent;
            if (__x) __x->_M_parent = __y->_M_parent;
            __y->_M_parent->_M_left = __x;   // __y must be a child of _M_left
            __y->_M_right = __z->_M_right;
            __z->_M_right->_M_parent = __y;
        }
        else
        __x_parent = __y;
        if (__root == __z)
        __root = __y;
        else if (__z->_M_parent->_M_left == __z)
        __z->_M_parent->_M_left = __y;
        else
        __z->_M_parent->_M_right = __y;
        __y->_M_parent = __z->_M_parent;
        std::swap(__y->_M_color, __z->_M_color);
        __y = __z;
        // __y now points to node to be actually deleted
        }
        else
        {                        // __y == __z
        __x_parent = __y->_M_parent;
        if (__x)
        __x->_M_parent = __y->_M_parent;
        if (__root == __z)
        __root = __x;
        else
        if (__z->_M_parent->_M_left == __z)
            __z->_M_parent->_M_left = __x;
        else
            __z->_M_parent->_M_right = __x;
        if (__leftmost == __z)
        {
            if (__z->_M_right == 0)        // __z->_M_left must be null also
            __leftmost = __z->_M_parent;
            // makes __leftmost == _M_header if __z == __root
            else
            __leftmost = _Rb_tree_node_base::_S_minimum(__x);
        }
        if (__rightmost == __z)
        {
            if (__z->_M_left == 0)         // __z->_M_right must be null also
            __rightmost = __z->_M_parent;
            // makes __rightmost == _M_header if __z == __root
            else                      // __x == __z->_M_left
            __rightmost = _Rb_tree_node_base::_S_maximum(__x);
        }
        }
        if (__y->_M_color != _S_red)
        {
        while (__x != __root && (__x == 0 || __x->_M_color == _S_black))
        if (__x == __x_parent->_M_left)
            {
            _Rb_tree_node_base* __w = __x_parent->_M_right;
            if (__w->_M_color == _S_red)
            {
            __w->_M_color = _S_black;
            __x_parent->_M_color = _S_red;
            local_Rb_tree_rotate_left(__x_parent, __root);
            __w = __x_parent->_M_right;
            }
            if ((__w->_M_left == 0 ||
            __w->_M_left->_M_color == _S_black) &&
            (__w->_M_right == 0 ||
            __w->_M_right->_M_color == _S_black))
            {
            __w->_M_color = _S_red;
            __x = __x_parent;
            __x_parent = __x_parent->_M_parent;
            }
            else
            {
            if (__w->_M_right == 0
                || __w->_M_right->_M_color == _S_black)
                {
                __w->_M_left->_M_color = _S_black;
                __w->_M_color = _S_red;
                local_Rb_tree_rotate_right(__w, __root);
                __w = __x_parent->_M_right;
                }
            __w->_M_color = __x_parent->_M_color;
            __x_parent->_M_color = _S_black;
            if (__w->_M_right)
                __w->_M_right->_M_color = _S_black;
            local_Rb_tree_rotate_left(__x_parent, __root);
            break;
            }
            }
        else
            {
            // same as above, with _M_right <-> _M_left.
            _Rb_tree_node_base* __w = __x_parent->_M_left;
            if (__w->_M_color == _S_red)
            {
            __w->_M_color = _S_black;
            __x_parent->_M_color = _S_red;
            local_Rb_tree_rotate_right(__x_parent, __root);
            __w = __x_parent->_M_left;
            }
            if ((__w->_M_right == 0 ||
            __w->_M_right->_M_color == _S_black) &&
            (__w->_M_left == 0 ||
            __w->_M_left->_M_color == _S_black))
            {
            __w->_M_color = _S_red;
            __x = __x_parent;
            __x_parent = __x_parent->_M_parent;
            }
            else
            {
            if (__w->_M_left == 0 || __w->_M_left->_M_color == _S_black)
                {
                __w->_M_right->_M_color = _S_black;
                __w->_M_color = _S_red;
                local_Rb_tree_rotate_left(__w, __root);
                __w = __x_parent->_M_left;
                }
            __w->_M_color = __x_parent->_M_color;
            __x_parent->_M_color = _S_black;
            if (__w->_M_left)
                __w->_M_left->_M_color = _S_black;
            local_Rb_tree_rotate_right(__x_parent, __root);
            break;
            }
            }
        if (__x) __x->_M_color = _S_black;
        }
        return __y;
    }

    namespace __detail {
        extern const unsigned long __prime_list[] = // 256 + 1 or 256 + 48 + 1
        {
            2ul, 3ul, 5ul, 7ul, 11ul, 13ul, 17ul, 19ul, 23ul, 29ul, 31ul,
            37ul, 41ul, 43ul, 47ul, 53ul, 59ul, 61ul, 67ul, 71ul, 73ul, 79ul,
            83ul, 89ul, 97ul, 103ul, 109ul, 113ul, 127ul, 137ul, 139ul, 149ul,
            157ul, 167ul, 179ul, 193ul, 199ul, 211ul, 227ul, 241ul, 257ul,
            277ul, 293ul, 313ul, 337ul, 359ul, 383ul, 409ul, 439ul, 467ul,
            503ul, 541ul, 577ul, 619ul, 661ul, 709ul, 761ul, 823ul, 887ul,
            953ul, 1031ul, 1109ul, 1193ul, 1289ul, 1381ul, 1493ul, 1613ul,
            1741ul, 1879ul, 2029ul, 2179ul, 2357ul, 2549ul, 2753ul, 2971ul,
            3209ul, 3469ul, 3739ul, 4027ul, 4349ul, 4703ul, 5087ul, 5503ul,
            5953ul, 6427ul, 6949ul, 7517ul, 8123ul, 8783ul, 9497ul, 10273ul,
            11113ul, 12011ul, 12983ul, 14033ul, 15173ul, 16411ul, 17749ul,
            19183ul, 20753ul, 22447ul, 24281ul, 26267ul, 28411ul, 30727ul,
            33223ul, 35933ul, 38873ul, 42043ul, 45481ul, 49201ul, 53201ul,
            57557ul, 62233ul, 67307ul, 72817ul, 78779ul, 85229ul, 92203ul,
            99733ul, 107897ul, 116731ul, 126271ul, 136607ul, 147793ul,
            159871ul, 172933ul, 187091ul, 202409ul, 218971ul, 236897ul,
            256279ul, 277261ul, 299951ul, 324503ul, 351061ul, 379787ul,
            410857ul, 444487ul, 480881ul, 520241ul, 562841ul, 608903ul,
            658753ul, 712697ul, 771049ul, 834181ul, 902483ul, 976369ul,
            1056323ul, 1142821ul, 1236397ul, 1337629ul, 1447153ul, 1565659ul,
            1693859ul, 1832561ul, 1982627ul, 2144977ul, 2320627ul, 2510653ul,
            2716249ul, 2938679ul, 3179303ul, 3439651ul, 3721303ul, 4026031ul,
            4355707ul, 4712381ul, 5098259ul, 5515729ul, 5967347ul, 6456007ul,
            6984629ul, 7556579ul, 8175383ul, 8844859ul, 9569143ul, 10352717ul,
            11200489ul, 12117689ul, 13109983ul, 14183539ul, 15345007ul,
            16601593ul, 17961079ul, 19431899ul, 21023161ul, 22744717ul,
            24607243ul, 26622317ul, 28802401ul, 31160981ul, 33712729ul,
            36473443ul, 39460231ul, 42691603ul, 46187573ul, 49969847ul,
            54061849ul, 58488943ul, 63278561ul, 68460391ul, 74066549ul,
            80131819ul, 86693767ul, 93793069ul, 101473717ul, 109783337ul,
            118773397ul, 128499677ul, 139022417ul, 150406843ul, 162723577ul,
            176048909ul, 190465427ul, 206062531ul, 222936881ul, 241193053ul,
            260944219ul, 282312799ul, 305431229ul, 330442829ul, 357502601ul,
            386778277ul, 418451333ul, 452718089ul, 489790921ul, 529899637ul,
            573292817ul, 620239453ul, 671030513ul, 725980837ul, 785430967ul,
            849749479ul, 919334987ul, 994618837ul, 1076067617ul, 1164186217ul,
            1259520799ul, 1362662261ul, 1474249943ul, 1594975441ul, 1725587117ul,
            1866894511ul, 2019773507ul, 2185171673ul, 2364114217ul, 2557710269ul,
            2767159799ul, 2993761039ul, 3238918481ul, 3504151727ul, 3791104843ul,
            4101556399ul, 4294967291ul,
            // Sentinel, so we don't have to test the result of lower_bound,
            // or, on 64-bit machines, rest of the table.
#if __SIZEOF_LONG__ != 8
            4294967291ul
#else
            6442450933ul, 8589934583ul, 12884901857ul, 17179869143ul,
            25769803693ul, 34359738337ul, 51539607367ul, 68719476731ul,
            103079215087ul, 137438953447ul, 206158430123ul, 274877906899ul,
            412316860387ul, 549755813881ul, 824633720731ul, 1099511627689ul,
            1649267441579ul, 2199023255531ul, 3298534883309ul, 4398046511093ul,
            6597069766607ul, 8796093022151ul, 13194139533241ul, 17592186044399ul,
            26388279066581ul, 35184372088777ul, 52776558133177ul, 70368744177643ul,
            105553116266399ul, 140737488355213ul, 211106232532861ul, 281474976710597ul,
            562949953421231ul, 1125899906842597ul, 2251799813685119ul,
            4503599627370449ul, 9007199254740881ul, 18014398509481951ul,
            36028797018963913ul, 72057594037927931ul, 144115188075855859ul,
            288230376151711717ul, 576460752303423433ul,
            1152921504606846883ul, 2305843009213693951ul,
            4611686018427387847ul, 9223372036854775783ul,
            18446744073709551557ul, 18446744073709551557ul
#endif
        };

        std::pair<bool, std::size_t>
        _Prime_rehash_policy::
        _M_need_rehash(std::size_t __n_bkt, std::size_t __n_elt,
                std::size_t __n_ins) const
        {
            if (__n_elt + __n_ins > _M_next_resize)
            {
            // If _M_next_resize is 0 it means that we have nothing allocated so
            // far and that we start inserting elements. In this case we start
            // with an initial bucket size of 11.
            double __min_bkts
            = std::max<std::size_t>(__n_elt + __n_ins, _M_next_resize ? 0 : 11)
            / (double)_M_max_load_factor;
            if (__min_bkts >= __n_bkt)
            return { true,
                _M_next_bkt(std::max<std::size_t>(__builtin_floor(__min_bkts) + 1,
                                __n_bkt * _S_growth_factor)) };

            _M_next_resize
            = __builtin_floor(__n_bkt * (double)_M_max_load_factor);
            return { false, 0 };
            }
            else
            return { false, 0 };
        }

        std::size_t
        _Prime_rehash_policy::_M_next_bkt(std::size_t __n) const
        {
            // Optimize lookups involving the first elements of __prime_list.
            // (useful to speed-up, eg, constructors)
            static const unsigned char __fast_bkt[]
            = { 2, 2, 2, 3, 5, 5, 7, 7, 11, 11, 11, 11, 13, 13 };

            if (__n < sizeof(__fast_bkt))
            {
            if (__n == 0)
            // Special case on container 1st initialization with 0 bucket count
            // hint. We keep _M_next_resize to 0 to make sure that next time we
            // want to add an element allocation will take place.
            return 1;

            _M_next_resize =
            __builtin_floor(__fast_bkt[__n] * (double)_M_max_load_factor);
            return __fast_bkt[__n];
            }

            // Number of primes (without sentinel).
            constexpr auto __n_primes
            = sizeof(__prime_list) / sizeof(unsigned long) - 1;

            // Don't include the last prime in the search, so that anything
            // higher than the second-to-last prime returns a past-the-end
            // iterator that can be dereferenced to get the last prime.
            constexpr auto __last_prime = __prime_list + __n_primes - 1;

            const unsigned long* __next_bkt =
            std::lower_bound(__prime_list + 6, __last_prime, __n);

            if (__next_bkt == __last_prime)
            // Set next resize to the max value so that we never try to rehash again
            // as we already reach the biggest possible bucket number.
            // Note that it might result in max_load_factor not being respected.
            _M_next_resize = size_t(-1);
            else
            _M_next_resize =
            __builtin_floor(*__next_bkt * (double)_M_max_load_factor);

            return *__next_bkt;
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
