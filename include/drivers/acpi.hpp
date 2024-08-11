#pragma once
#include <uacpi/tables.h>
#include <cstdlib>
#include <util/logger.hpp>

namespace acpi {
    void init();

    template <class T>
    T get_table(const char *sig) {
        uacpi_table t;
        if (uacpi_unlikely(uacpi_table_find_by_signature(sig, &t))) {
            logger::error("uACPI", "uACPI couldn't find table with signature '%s'", T::SIGNATURE);
            abort();
        }

        return T{t.ptr};
    }

    class SDT {
    protected:
        explicit SDT(const acpi_sdt_hdr*);
    
        static bool validate(const char*, size_t);
        const acpi_sdt_hdr *header;
    
    private:
        bool validate() const;
    };
}