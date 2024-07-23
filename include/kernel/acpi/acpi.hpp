#pragma once
#include <uacpi/tables.h>
#include <cstdlib>
#include <util/logger.h>

namespace acpi {
    template <class T>
    T get_table() {
        uacpi_table t;
        if (uacpi_unlikely(uacpi_table_find_by_signature(T::SIGNATURE, &t))) {
            log(Error, "uACPI", "uACPI couldn't find table with signature '%s'", T::SIGNATURE);
            abort();
        }

        return T{t.ptr};
    }
}