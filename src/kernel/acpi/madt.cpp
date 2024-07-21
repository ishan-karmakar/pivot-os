#include <acpi/madt.hpp>
#include <util/logger.h>
using namespace acpi;

template <class E>
MADT::Iterator<E>::Iterator(const madt * const header) : ptr{reinterpret_cast<pointer>(header + 1)}, end{reinterpret_cast<uintptr_t>(header) + header->length} {
    if (ptr->type != E::TYPE)
        ++*this;
}

template <class E>
void MADT::Iterator<E>::operator++() {
    do {
        ptr = reinterpret_cast<pointer>(reinterpret_cast<const char*>(ptr) + ptr->length);

        if (ptr->type == E::TYPE)
            break;
    } while (reinterpret_cast<uintptr_t>(ptr) < end);
}

template <class E>
MADT::Iterator<E>::operator bool() const {
    return reinterpret_cast<uintptr_t>(ptr) < end;
}

template <class E>
typename MADT::Iterator<E>::pointer MADT::Iterator<E>::operator->() const { return ptr; }

template <class E>
typename MADT::Iterator<E>::value_type MADT::Iterator<E>::operator*() const { return *ptr; }

template class MADT::Iterator<MADT::lapic>;
template class MADT::Iterator<MADT::ioapic>;
template class MADT::Iterator<MADT::ioapic_so>;
