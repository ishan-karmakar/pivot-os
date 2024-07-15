#include <acpi/acpi.hpp>
#include <acpi/madt.hpp>
#include <util/logger.h>
#include <libc/string.h>
using namespace acpi;

SDT::SDT(SDT::sdt *header) : header{header} {
    if (!validate())
        log(Warning, "ACPI", "Found invalid ACPI table");
}

bool SDT::validate(char *header, uint32_t length) {
    uint8_t sum = 0;
    for (uint32_t i = 0; i < length; i++)
        sum += header[i];
    return sum == 0;
}

bool SDT::validate() {
    return validate(reinterpret_cast<char*>(header), header->length);
}

RSDT::RSDT(uintptr_t rsdp) : SDT{get_rsdt(reinterpret_cast<char*>(rsdp))} {
    log(Info, "ACPI", "Found %cSDT table", xsdt ? 'X' : 'R');
}

template <class T>
std::optional<T> RSDT::get_table() {
    uint32_t num_entries = (header->length - sizeof(SDT::sdt)) / (xsdt ? sizeof(uint64_t) : sizeof(uint32_t));
    auto start = reinterpret_cast<uintptr_t>(header + 1);
    for (uint32_t i = 0; i < num_entries; i++) {
        uintptr_t addr;
        if (xsdt)
            addr = reinterpret_cast<uint64_t*>(start)[i];
        else
            addr = reinterpret_cast<uint32_t*>(start)[i];
        auto table = reinterpret_cast<SDT::sdt*>(addr);
        if (!memcmp(table->sig, T::SIGNATURE, sizeof(table->sig)))
            return std::make_optional(T{table});
    }
    return std::nullopt;
}

SDT::sdt *RSDT::get_rsdt(char *sdp) {
    auto rsdp = reinterpret_cast<RSDT::rsdp*>(sdp);
    xsdt = false;
    if (!(!memcmp(rsdp->signature, "RSD PTR ", sizeof(rsdp->signature)) && validate(sdp, sizeof(RSDT::rsdp))))
        log(Warning, "ACPI", "RSDP is not valid");
    
    if (rsdp->revision != 2)
        return reinterpret_cast<SDT::sdt*>(rsdp->rsdt_addr);
    
    xsdt = true;
    if (!validate(sdp, rsdp->length))
        log(Warning, "ACPI", "XSDP is not valid");
    return reinterpret_cast<SDT::sdt*>(rsdp->xsdt_addr);
}

template std::optional<MADT> RSDT::get_table();
