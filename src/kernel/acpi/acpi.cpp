#include <acpi/acpi.hpp>
#include <util/logger.h>
#include <libc/string.h>
using namespace acpi;

SDT::SDT(struct sdt *header) : header{header} {
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
    uint32_t num_entries = (header->length - sizeof(struct sdt)) / (xsdt ? sizeof(uint64_t) : sizeof(uint32_t));
    uintptr_t cur_table = reinterpret_cast<uintptr_t>(header + 1);
    for (uint32_t i = 0; i < num_entries; i++) {
        struct sdt *table = reinterpret_cast<struct sdt*>(cur_table);
        // if (!memcmp(table->sig, T::SIGNATURE, sizeof(table->sig)))
        //     return std::make_optional(T{table});
        cur_table += xsdt ? sizeof(uint64_t) : sizeof(uint32_t);
    }
    return std::nullopt;
}

struct SDT::sdt *RSDT::get_rsdt(char *sdp) {
    auto rsdp = reinterpret_cast<struct rsdp*>(sdp);
    xsdt = false;
    if (!(!memcmp(rsdp->signature, "RSD PTR ", sizeof(rsdp->signature)) && validate(sdp, sizeof(struct rsdp))))
        log(Warning, "ACPI", "RSDP is not valid");
    
    if (rsdp->revision != 2)
        return reinterpret_cast<struct sdt*>(rsdp->rsdt_addr);
    
    xsdt = true;
    auto xsdp = reinterpret_cast<struct xsdp*>(sdp);
    if (!validate(sdp, xsdp->length))
        log(Warning, "ACPI", "XSDP is not valid");
    return reinterpret_cast<struct sdt*>(xsdp->xsdt_addr);
}

template std::optional<MADT> RSDT::get_table();
