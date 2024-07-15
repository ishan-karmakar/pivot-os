#pragma once
#include <boot.h>
#include <optional>

namespace acpi {
    class SDT {
    protected:
        struct [[gnu::packed]] sdt {
            char sig[4];
            uint32_t length;
            uint8_t revision;
            uint8_t checksum;
            char oemid[6];
            char oemtableid[8];
            uint32_t oem_revision;
            uint32_t creator_id;
            uint32_t creator_revision;
        };

        SDT(sdt*);
        static bool validate(char*, uint32_t);
        sdt *header;

        private:
            bool validate();
    };

    class ACPI : SDT {
    public:
        ACPI(uintptr_t);

        template <class T>
        std::optional<T> get_table();

    private:
        struct [[gnu::packed]] rsdp {
            char signature[8];
            uint8_t checksum;
            char oemid[6];
            uint8_t revision;
            uint32_t rsdt_addr;

            uint32_t length;
            uint64_t xsdt_addr;
            uint8_t ext_checksum;
            uint8_t rsv[3];
        };

        sdt *get_rsdt(char*);
        bool xsdt;
    };
}