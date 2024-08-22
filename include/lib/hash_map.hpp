#pragma once
#include <frg/hash_map.hpp>
#include <mem/heap.hpp>

namespace lib {
    template <typename K, typename V, typename H = frg::hash<K>>
    class hash_map : public frg::hash_map<K, V, H, heap::allocator_t> {};
}