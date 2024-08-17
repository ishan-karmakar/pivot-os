#pragma once
#include <uacpi/tables.h>
#include <cstdlib>
#include <lib/logger.hpp>

namespace acpi {
    void init();

    template <class T>
    T get_table(const char *sig) {
        uacpi_table t;
        ASSERT(uacpi_likely_success(uacpi_table_find_by_signature(sig, &t)));

        return T{t.hdr};
    }

    class SDT {
    protected:
        explicit SDT(const acpi_sdt_hdr*);
    
        static bool validate(const char*, std::size_t);
        const acpi_sdt_hdr *header;
    
    private:
        bool validate() const;
    };
}