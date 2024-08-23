#pragma once
#include <drivers/acpi.hpp>
#include <uacpi/acpi.h>
#include <iterator>
#include <frg/vector.hpp>
#include <mem/heap.hpp>

namespace acpi {
    class madt : sdt {
    private:
        template <class E>
        class Iterator {
        public:
            using iterator_category = std::input_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = E;
            using pointer = const value_type*;
            using reference = const value_type&;

            Iterator(const acpi_madt *table, acpi_madt_entry_type type) : ptr{reinterpret_cast<pointer>(table + 1)}, end{reinterpret_cast<uintptr_t>(table) + table->hdr.length}, target_type{type} {
                if (ptr->hdr.type != target_type)
                    ++*this;
            }

            reference operator*() const { return *ptr; }
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

        const acpi_madt *table;

    public:
        madt(const acpi_sdt_hdr*);

        uintptr_t lapic_addr;
        frg::vector<const acpi_madt_interrupt_source_override*, heap::allocator_t> source_ovrds;
        frg::vector<const acpi_madt_ioapic*, heap::allocator_t> ioapics;
    };

    extern class madt *madt;
}