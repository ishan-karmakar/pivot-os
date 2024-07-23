#pragma once
#include <uacpi/acpi.h>
#include <uacpi/tables.h>
#include <iterator>

namespace acpi {
    class MADT {
    public:
        template <class E>
        class Iterator {
        public:
            using iterator_category = std::input_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = E;
            using pointer = const value_type*;
            using reference = const value_type&;

            Iterator(const acpi_madt *table, uint8_t type) : type{type}, ptr{reinterpret_cast<pointer>(table + 1)}, end{reinterpret_cast<uintptr_t>(table) + table->hdr.length} {
                if (ptr->hdr.type != type)
                    ++*this;
            }

            void operator++() {
                do {
                    ptr = reinterpret_cast<pointer>(reinterpret_cast<const char*>(ptr) + ptr->hdr.length);

                    if (ptr->hdr.type == type)
                        break;
                } while (reinterpret_cast<uintptr_t>(ptr) < end);
            }

            operator bool() const { return reinterpret_cast<uintptr_t>(ptr) < end; }
            pointer operator->() const { return ptr; }
            value_type operator*() const { return *ptr; }

        private:
            uint8_t type;
            pointer ptr;
            uintptr_t end;
        };

        MADT(void *h) : table{static_cast<const acpi_madt*>(h)} {};

        template <class E>
        Iterator<E> iter(uint8_t t) const { return Iterator<E>{table, t}; };

        const acpi_madt *table;

        static constexpr const char *SIGNATURE = "APIC";
    };
}