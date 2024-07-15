#include <acpi/madt.hpp>
#include <util/logger.h>
using namespace acpi;

MADT::MADT(SDT::sdt *header) : SDT{header}, table{reinterpret_cast<madt*>(header)} {}

template <class E>
MADT::MADTIterator<E>::MADTIterator(madt *header) : end{reinterpret_cast<uintptr_t>(header) + header->header.length} {
    ptr = reinterpret_cast<pointer>(header + 1);
    if (ptr->header.type != E::TYPE)
        ++*this;
}

template <class E>
void MADT::MADTIterator<E>::operator++() {
    do {
        ptr = reinterpret_cast<pointer>(reinterpret_cast<char*>(ptr) + ptr->header.length);

        if (ptr->header.type == E::TYPE)
            break;
    } while (reinterpret_cast<uintptr_t>(ptr) < end);
}

template <class E>
MADT::MADTIterator<E>::operator bool() {
    return reinterpret_cast<uintptr_t>(ptr) < end;
}

template <class E>
typename MADT::MADTIterator<E>::pointer MADT::MADTIterator<E>::operator->() const { return ptr; }

template <class E>
typename MADT::MADTIterator<E>::reference MADT::MADTIterator<E>::operator*() const { return *ptr; }

template class MADT::MADTIterator<MADT::madt_lapic>;