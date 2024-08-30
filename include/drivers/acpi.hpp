#pragma once
#include <uacpi/tables.h>
#include <cstdlib>
#include <lib/logger.hpp>

namespace acpi {
    void init();
    [[noreturn]] void shutdown();

    acpi_sdt_hdr *get_table(const char *sig);

    class sdt {
    protected:
        explicit sdt(const acpi_sdt_hdr *hdr) : header{hdr} {};
    
        static bool validate(const char*, std::size_t);
        const acpi_sdt_hdr *header;
    
    private:
        bool validate() const;
    };
}