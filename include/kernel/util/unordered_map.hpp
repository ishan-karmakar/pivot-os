// #pragma once
// #include <util/string.hpp>
// #include <util/hash.hpp>
// #include <cstdlib>

// namespace util {
//     template <typename K, typename V>
//     class UnorderedMap {
//     public:
//         UnorderedMap() = default;
//         UnorderedMap(size_t init_size) : size{init_size}, table{new node*[init_size]()} {}
//         ~UnorderedMap() {
//             for (size_t i = 0; i < size; i++) {
//                 node *n = table[i];
//                 while (n) {
//                     node *t = n;
//                     n = n->next;
//                     delete t;
//                 }
//             }
//             delete[] table;
//         }

//         void insert(const K& key, const V& value) {
//             size_t idx = hasher(key) % size;

//             node *n = table[idx];
//             node *p = nullptr;
//             while (n) {
//                 if (n->key == key) {
//                     n->value = value;
//                     return;
//                 }
//                 p = n;
//                 n = n->next;
//             }

//             node *nn = new node;
//             nn->key = key;
//             nn->value = value;
//             nn->next = nullptr;

//             if (p)
//                 p->next = nn;
//             else
//                 table[idx] = nn;
            
//             if (should_resize(idx))
//                 resize();
//         }

//         V& operator[](const K& key) {
//             size_t idx = hasher(key) % size;
//             node *n = table[idx];
//             while (n) {
//                 if (n->key == key)
//                     return n->value;
//                 n = n->next;
//             }
//             insert(key, V{});
//             return (*this)[key];
//         }

//         bool find(const K& key) const {
//             size_t idx = hasher(key) % size;
//             node *n = table[idx];
//             while (n) {
//                 if (n->key == key)
//                     return true;
//                 n = n->next;
//             }
//             return false;
//         }

//         void remove(const K& key) {
//             size_t idx = hasher(key) % size;
//             node *n = table[idx];
//             node *p = nullptr;
//             while (n) {
//                 if (n->key == key) {
//                     if (p)
//                         p->next = n->next;
//                     else
//                         table[idx] = n->next;
//                     delete n;
//                 }
//                 p = n;
//                 n = n->next;
//             }
//         }

//     private:
//         void resize() {
//             size_t new_size = size * 2;
//             node **new_tbl = new node*[new_size]();
//             for (size_t i = 0; i < size; i++) {
//                 node *n = table[i];
//                 while (n) {
//                     node *next = n->next;
//                     size_t new_idx = hasher(n->key) % new_size;
//                     n->next = new_tbl[new_idx];
//                     new_tbl[new_idx] = n;
//                     n = next;
//                 }
//             }

//             delete[] table;
//             table = new_tbl;
//             size = new_size;
//         }

//         bool should_resize(size_t idx) const {
//             node *n = table[idx];
//             size_t count = 0;
//             while (n) {
//                 count++;
//                 // Since we are calling this before inserting, we actually want to increase count by one manually
//                 if (count > RESIZE_THRESHOLD)
//                     return true;
//                 n = n->next;
//             }
//             return false;
//         }

//         struct node {
//             K key;
//             V value;
//             node *next;
//         };

//         size_t size;
//         node **table;
//         int ecount;
//         hash<K> hasher;

//         static constexpr int RESIZE_THRESHOLD = 2;
//     };
// }
