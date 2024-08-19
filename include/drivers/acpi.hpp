#pragma once
#include <uacpi/tables.h>
#include <cstdlib>
#include <lib/logger.hpp>

namespace acpi {
    void init();

    acpi_sdt_hdr *get_table(const char *sig);

    class SDT {
    protected:
        explicit SDT(const acpi_sdt_hdr*);
    
        static bool validate(const char*, std::size_t);
        const acpi_sdt_hdr *header;
    
    private:
        bool validate() const;
    };
}