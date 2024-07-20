#pragma once
#include <cstddef>
#include <util/hash.hpp>
#include <util/logger.h>
#include <libc/string.h>
#define UNORDERED_MAP_RESIZE_THRESHOLD 2

namespace util {
    template <typename K, typename V>
    class unordered_map {
    public:
        unordered_map() = default;
        unordered_map(size_t init_size) : arr{new item*[init_size]}, len{init_size} {
            memset(arr, 0, init_size * sizeof(*arr));
        }

        V& operator[](const K& key) {
            size_t h = hasher(key) % len;
            log(Verbose, "UMAP", "L: %u", len);
            log(Verbose, "UMAP", "%u", h);
            item *p = arr[h];
            size_t i = 0;
            while (p) {
                if (p->key == key)
                    return p->val;
                p = p->next;
                i++;
            }
            log(Verbose, "UMAP", "%lu", i);

            if (i > UNORDERED_MAP_RESIZE_THRESHOLD)
                resize();

            log(Verbose, "UMAP", "Creating default item");
            // Create default item
            p = arr[h];
            arr[h] = new item;
            arr[h]->key = key;
            arr[h]->val = V{};
            arr[h]->next = p;
            return arr[h]->val;
        }

    private:
        void resize() {
            log(Verbose, "UMAP", "Time to resize");
            size_t new_size = len * 2;
            item **new_arr = new item*[new_size]();
            for (size_t i = 0; i < len; i++) {
                item *e = arr[i];
                if (!e) continue;
                log(Verbose, "UMAP", "E: %s", e->key.c_str());
                while(1);
                size_t new_hash = hasher(e->key) % new_size;
                item *q = new_arr[new_hash];
                item *t = e;
                size_t nq, ne;
                nq = ne = 0;
                while (q) {
                    q = q->next;
                    nq++;
                    if (t) {
                        ne++;
                        t = t->next;
                    }
                }
                if ((nq + ne) < UNORDERED_MAP_RESIZE_THRESHOLD)
                    return resize();
                if (q)
                    q->next = e;
                else
                    new_arr[new_hash] = e;
            }
            delete[] arr;
            arr = new_arr;
            len = new_size;
        }

        struct item {
            K key;
            V val;
            item *next;
        };
        item **arr;
        size_t len;
        util::hash<K> hasher;
    };
}
