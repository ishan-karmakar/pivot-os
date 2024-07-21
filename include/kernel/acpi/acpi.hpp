#pragma once
#include <boot.h>
#include <optional>
#include <util/unordered_map.hpp>
#include <util/string.hpp>

/*
This whole setup is really janky. Basically, SDT is the base class for all
ACPI tables. It provides validation functions and allows passing around tables easily.
RSDT is the Root SDT, and it is different from the rest of the tables because it is the entry point
and needs additional functions to access the rest of tables. ACPI is kind of a wrapper class that facilitates
the initialization of RSDT because I wanted the kernel to be able to access ACPI with static functions.
*/
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

        SDT() = default;
        SDT(const sdt*);

    protected:
        static bool validate(const char* const, uint32_t);
        const sdt *header;

    private:
        bool validate() const;
    };

    class ACPI : SDT {
    public:
        // This should be called in the kernel, not the constructors
        ACPI() = default;
        static void init(uintptr_t);
        template <class T>
        static std::optional<const T> get_table();

    protected:
        ACPI(uintptr_t);

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

        const SDT::sdt *parse_rsdp(const char*);
        bool xsdt;
        util::UnorderedMap<util::String, sdt*> tables;
        static ACPI *rsdt;
    };
}