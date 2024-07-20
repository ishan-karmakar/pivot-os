#include <acpi/acpi.hpp>
#include <acpi/madt.hpp>
#include <util/logger.h>
using namespace acpi;

ACPI rsdt;

SDT::SDT(const sdt *header) : header{header} {
    if (!validate())
        log(Warning, "ACPI", "Found invalid ACPI table");
}

bool SDT::validate(const char *header, uint32_t length) {
    uint8_t sum = 0;
    for (uint32_t i = 0; i < length; i++)
        sum += header[i];
    return sum == 0;
}

bool SDT::validate() const {
    return validate(reinterpret_cast<const char*>(header), header->length);
}

ACPI::ACPI(uintptr_t rsdp) : SDT{parse_rsdp(reinterpret_cast<const char*>(rsdp))}, tables{4} {
    log(Info, "ACPI", "Found %cSDT table", xsdt ? 'X' : 'R');
    uint32_t num_entries = (header->length - sizeof(SDT::sdt)) / (xsdt ? sizeof(uint64_t) : sizeof(uint32_t));
    auto start = reinterpret_cast<uintptr_t>(header + 1);
    for (uint32_t i = 0; i < num_entries; i++) {
        uintptr_t addr;
        if (xsdt)
            addr = reinterpret_cast<uint64_t*>(start)[i];
        else
            addr = reinterpret_cast<uint32_t*>(start)[i];
        auto table = reinterpret_cast<sdt*>(addr);
        
        if (validate(reinterpret_cast<const char*>(table), table->length)) {
            util::String sig{table->sig, 4};
            log(Verbose, "ACPI", "Found valid %s", sig.c_str());
            tables.insert(sig, table);
        } else {
            log(Warning, "ACPI", "Found invalid table");
        }
    }
}

void ACPI::init(uintptr_t rsdp) {
    rsdt = ACPI{rsdp};
}

template <class T>
std::optional<const T> ACPI::get_table() {
    log(Verbose, "ACPI", "%hhu", rsdt.tables.find(T::SIGNATURE));
    if (rsdt.tables.find(T::SIGNATURE))
        return T{rsdt.tables[T::SIGNATURE]};
    return std::nullopt;
}

const SDT::sdt *ACPI::parse_rsdp(const char *sdp) {
    auto rsdp = reinterpret_cast<const ACPI::rsdp*>(sdp);
    if (!(!memcmp(rsdp->signature, "RSD PTR ", sizeof(rsdp->signature)) && validate(sdp, sizeof(ACPI::rsdp))))
        log(Warning, "ACPI", "RSDP is not valid");

    xsdt = false;
    if (rsdp->revision != 2)
        return reinterpret_cast<const SDT::sdt*>(rsdp->rsdt_addr);
    
    if (!validate(sdp, rsdp->length))
        log(Warning, "ACPI", "XSDP is not valid");
    xsdt = true;
    return reinterpret_cast<const SDT::sdt*>(rsdp->xsdt_addr);
}

template std::optional<const MADT> ACPI::get_table();
