#pragma once
#include <frg/hash_map.hpp>
#include <mem/heap.hpp>

namespace lib {
    template <typename K, typename V, typename H = frg::hash<K>, typename A = heap::allocator_t>
    // using hash_map = frg::hash_map<K, V, H, A>;
    class hash_map : public frg::hash_map<K, V, H, A> {
        using entry_type = frg::hash_map<K, V, H, A>::entry_type;
    public:
        constexpr hash_map(const H& h = H(), A a = A()) : frg::hash_map<K, V, H, A>{h, a} {}
        hash_map(std::initializer_list<entry_type> il, const H& h = H(), A a = A()) : frg::hash_map<K, V, H, A>{h, il, a} {};
    };
}