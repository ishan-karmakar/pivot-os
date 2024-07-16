#include <acpi/madt.hpp>
#include <util/logger.h>
using namespace acpi;

MADT::MADT(const SDT::sdt * const header) : SDT{header}, table{reinterpret_cast<const madt*>(header)} {}

template <class E>
MADT::MADTIterator<E>::MADTIterator(const madt * const header) : ptr{reinterpret_cast<pointer>(header + 1)}, end{reinterpret_cast<uintptr_t>(header) + header->length} {
    if (ptr->type != E::TYPE)
        ++*this;
}

template <class E>
void MADT::MADTIterator<E>::operator++() {
    do {
        ptr = reinterpret_cast<pointer>(reinterpret_cast<const char*>(ptr) + ptr->length);

        if (ptr->type == E::TYPE)
            break;
    } while (reinterpret_cast<uintptr_t>(ptr) < end);
}

template <class E>
MADT::MADTIterator<E>::operator bool() const {
    return reinterpret_cast<uintptr_t>(ptr) < end;
}

template <class E>
typename MADT::MADTIterator<E>::pointer MADT::MADTIterator<E>::operator->() const { return ptr; }

template <class E>
typename MADT::MADTIterator<E>::reference MADT::MADTIterator<E>::operator*() const { return *ptr; }

template class MADT::MADTIterator<MADT::lapic>;
template class MADT::MADTIterator<MADT::ioapic>;
