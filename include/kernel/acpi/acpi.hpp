#pragma once
#include <boot.h>
#include <optional>

namespace acpi {
    class SDT {
    public:
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

        SDT(struct sdt*);
        static bool validate(char*, uint32_t);
        struct sdt *header;

        private:
            bool validate();
    };

    class MADT : SDT {
    public:
        MADT(struct sdt* h) : SDT{h} {};
        static constexpr const char * SIGNATURE = "APIC";
    };

    class RSDT : SDT {
    public:
        RSDT(uintptr_t);

        template <class T>
        std::optional<T> get_table();

    private:
        struct [[gnu::packed]] rsdp {
            char signature[8];
            uint8_t checksum;
            char oemid[6];
            uint8_t revision;
            uint32_t rsdt_addr;
        };

        struct [[gnu::packed]] xsdp {
            struct rsdp rsdp;
            uint32_t length;
            uint64_t xsdt_addr;
            uint8_t ext_checksum;
            uint8_t rsv[3];
        };

        struct sdt *get_rsdt(char*);
        bool xsdt;
    };
}