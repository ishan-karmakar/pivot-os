#pragma once
#include <acpi/acpi.hpp>

namespace acpi {
    class MADT : SDT {
    public:
        struct [[gnu::packed]] madt {
            struct sdt header;
            uint32_t lapic_addr;
            uint32_t flags;
        };

        struct [[gnu::packed]] madt_header {
            uint8_t type;
            uint8_t length;
        };

        struct [[gnu::packed]] madt_lapic {
            madt_header header;
            uint8_t acpi_id;
            uint8_t apic_id;
            uint32_t flags;

            static constexpr uint8_t TYPE = 0;
        };

        template <class E>
        class MADTIterator {
        public:
            using iterator_category = std::input_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = E;
            using pointer = E*;
            using reference = E&;

            MADTIterator(madt *header);

            reference operator*() const;
            pointer operator->() const;

            void operator++();
            operator bool();

        private:
            pointer ptr;
            uintptr_t end;
        };

        MADT(struct sdt* h);

        template <class E>
        MADTIterator<E> iter() { return MADTIterator<E>{table}; };

        static constexpr const char *SIGNATURE = "APIC";

    private:
        madt *table;
    };
}