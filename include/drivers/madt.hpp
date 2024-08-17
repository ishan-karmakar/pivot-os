#pragma once
#include <drivers/acpi.hpp>
#include <uacpi/acpi.h>
#include <iterator>

namespace acpi {
    class MADT : SDT {
    public:
        template <class E>
        class Iterator {
        public:
            using iterator_category = std::input_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = E;
            using pointer = const value_type*;
            using reference = const value_type&;

            Iterator(const acpi_madt *header, acpi_madt_entry_type type) : ptr{reinterpret_cast<pointer>(header + 1)}, end{reinterpret_cast<uintptr_t>(header) + header->hdr.length}, target_type{type} {
                if (ptr->hdr.type != target_type)
                    ++*this;
            }

            value_type operator*() const { return *ptr; }
            pointer operator->() const { return ptr; }

            void operator++() {
                do {
                    ptr = reinterpret_cast<pointer>(reinterpret_cast<const char*>(ptr) + ptr->hdr.length);
                    if (ptr->hdr.type == target_type) break;
                } while (*this);
            }

            operator bool() const {
                return reinterpret_cast<uintptr_t>(ptr) < end;
            }

        private:
            pointer ptr;
            uintptr_t end;
            acpi_madt_entry_type target_type;
        };

        MADT(const acpi_sdt_hdr *tbl) : SDT{tbl}, table{reinterpret_cast<const acpi_madt*>(tbl)} {}

        template <class E>
        Iterator<E> iter(acpi_madt_entry_type t) const { return Iterator<E>{table, t}; };

        const acpi_madt *table;
    };
}