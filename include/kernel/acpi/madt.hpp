#pragma once
#include <acpi/acpi.hpp>

namespace acpi {
    class MADT : SDT {
    public:
        struct [[gnu::packed]] madt : public sdt {
            uint32_t lapic_addr;
            uint32_t flags;
        };

        struct [[gnu::packed]] header {
            uint8_t type;
            uint8_t length;
        };

        struct [[gnu::packed]] lapic : public header {
            uint8_t acpi_id;
            uint8_t apic_id;
            uint32_t flags;

            static constexpr uint8_t TYPE = 0;
        };

        struct [[gnu::packed]] ioapic : public header {
            uint8_t id;
            uint8_t rsv;
            uint32_t addr;
            uint32_t gsi_base;

            static constexpr uint8_t TYPE = 1;
        };

        struct [[gnu::packed]] ioapic_so : public header {
            uint8_t bus_source;
            uint8_t irq_source;
            uint32_t gsi_base;
            uint16_t flags;

            static constexpr uint8_t TYPE = 2;
        };

        template <class E>
        class Iterator {
        public:
            using iterator_category = std::input_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = E;
            using pointer = const value_type*;
            using reference = const value_type&;

            Iterator(const madt * const header);

            value_type operator*() const;
            pointer operator->() const;

            void operator++();
            operator bool() const;

        private:
            pointer ptr;
            uintptr_t end;
        };

        MADT(const sdt *h) : SDT{h}, table{reinterpret_cast<const madt*>(h)} {};

        template <class E>
        Iterator<E> iter() const { return Iterator<E>{table}; };

        const madt *table;

        static constexpr const char *SIGNATURE = "APIC";
    };
}