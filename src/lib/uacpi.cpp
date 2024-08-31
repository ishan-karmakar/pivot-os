#include <uacpi/kernel_api.h>
#include <cstddef>
#include <cpu/cpu.hpp>
#include <cpu/idt.hpp>
#include <frg/string.hpp>
#include <lib/timer.hpp>
#include <frg/spinlock.hpp>
#include <mem/mapper.hpp>
#include <mem/pmm.hpp>
#include <atomic>
#include <kernel.hpp>
#include <mem/heap.hpp>
#include <lib/interrupts.hpp>
#include <lib/proc.hpp>
#include <drivers/pci.hpp>
#include <assert.h>

uacpi_status uacpi_kernel_install_interrupt_handler(uacpi_u32 irq, uacpi_interrupt_handler func, uacpi_handle ctx, uacpi_handle *out_handle) {
    idt::handlers[irq].push_back([func, ctx](cpu::status*) {
        func(ctx);
        return nullptr;
    });
    *reinterpret_cast<std::size_t*>(out_handle) = irq;
    intr::set(intr::VEC(irq), irq);
    intr::unmask(irq);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_uninstall_interrupt_handler(uacpi_interrupt_handler, uacpi_handle handle) {
    idt::handlers[*reinterpret_cast<std::size_t*>(handle)].clear();
    return UACPI_STATUS_OK;
}

void uacpi_kernel_signal_event(uacpi_handle e) {
    (*static_cast<std::atomic_size_t*>(e))++;
}

void uacpi_kernel_reset_event(uacpi_handle e) {
    *static_cast<std::atomic_size_t*>(e) = 0;
}

uacpi_bool uacpi_kernel_wait_for_event(uacpi_handle e, uacpi_u16) {
    auto event = static_cast<std::atomic_size_t*>(e);
    while (*event == 0) asm ("pause");
    *event -= 1;
    return UACPI_TRUE;
}

uacpi_status uacpi_kernel_handle_firmware_request(uacpi_firmware_request *request) {
    switch (request->type) {
    case UACPI_FIRMWARE_REQUEST_TYPE_BREAKPOINT:
        logger::warning("UACPI[FM_REQ]", "Ignoring AML breakpoint - CTX: %p", request->breakpoint.ctx);
        break;
    
    case UACPI_FIRMWARE_REQUEST_TYPE_FATAL:
        logger::error("UACPI", "Fatal firmware error - type: %hhu, code: %u, arg: %lu", request->fatal.type, request->fatal.code, request->fatal.arg);
        break;
    }
    return UACPI_STATUS_OK;
}

void uacpi_kernel_vlog(uacpi_log_level log_level, const char *str, va_list args) {
    frg::string string{str, heap::allocator{}};
    string.resize(string.size() - 1);
    logger::vlog(static_cast<logger::log_level>(log_level), "UACPI", string.data(), args);
}

void uacpi_kernel_log(uacpi_log_level log_level, const char *str, ...) {
    va_list args;
    va_start(args, str);
    uacpi_kernel_vlog(log_level, str, args);
    va_end(args);
}

void uacpi_kernel_stall(uacpi_u8) {
    // us can be max of 256 so we can just sleep for 1 millisecond
    // TODO: Support microsecond level sleeping
    uacpi_kernel_sleep(1);
}

[[gnu::alias("uacpi_kernel_create_mutex")]] uacpi_handle uacpi_kernel_create_spinlock();
uacpi_handle uacpi_kernel_create_mutex() {
    return new std::atomic_flag;
}

bool uacpi_kernel_acquire_mutex(void *m, uint16_t tm) {
    auto flag = static_cast<std::atomic_flag*>(m);
    if (tm == 0xFFFF) {
        while (flag->test_and_set()) asm ("pause");
        return true;
    } else {
        std::size_t end = timer::time() + tm;
        while (flag->test_and_set())
            if (timer::time() >= end) return false;
        return true;
    }
}

void uacpi_kernel_release_mutex(void *m) {
    static_cast<frg::simple_spinlock*>(m)->unlock();
}

[[gnu::alias("uacpi_kernel_free_mutex")]] void uacpi_kernel_free_spinlock(uacpi_handle);
void uacpi_kernel_free_mutex(uacpi_handle m) {
    delete static_cast<frg::simple_spinlock*>(m);
}

uacpi_cpu_flags uacpi_kernel_spinlock_lock(uacpi_handle h) {
    asm volatile ("cli");
    uacpi_kernel_acquire_mutex(h, 0xFFFF);
    return 0;
}

void uacpi_kernel_spinlock_unlock(uacpi_handle h, uacpi_cpu_flags) {
    uacpi_kernel_release_mutex(h);
    asm volatile ("sti");
}

uacpi_handle uacpi_kernel_create_event() {
    return new std::atomic_size_t;
}

void uacpi_kernel_free_event(uacpi_handle e) {
    delete static_cast<std::atomic_size_t*>(e);
}

void *uacpi_kernel_map(uintptr_t phys, std::size_t) {
    uintptr_t virt = virt_addr(phys);
    mapper::kmapper->map(phys_addr(phys), virt, mapper::KERNEL_ENTRY);
    return reinterpret_cast<void*>(virt);
}

void uacpi_kernel_unmap(void *addr, std::size_t size) {
    mapper::kmapper->unmap(reinterpret_cast<uintptr_t>(virt_addr(addr)), div_ceil(size, PAGE_SIZE));
}

uacpi_status uacpi_kernel_raw_memory_read(uacpi_phys_addr address, uacpi_u8 byte_width, uacpi_u64 *out_value) {
    switch (byte_width) {
    case 1:
        *out_value = *(volatile uacpi_u8*) address;
        break;
    case 2:
        *out_value = *(volatile uacpi_u16*) address;
        break;
    case 4:
        *out_value = *(volatile uacpi_u32*) address;
        break;
    case 8:
        *out_value = *(volatile uacpi_u64*) address;
        break;
    }
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_raw_memory_write(uacpi_phys_addr address, uacpi_u8 byte_width, uacpi_u64 value) {
    switch (byte_width) {
    case 1:
        *(volatile uacpi_u8*) address = value;
        break;
    case 2:
        *(volatile uacpi_u16*) address = value;
        break;
    case 4:
        *(volatile uacpi_u32*) address = value;
        break;
    case 8:
        *(volatile uacpi_u64*) address = value;
        break;
    }
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_raw_io_read(uacpi_io_addr addr, uacpi_u8 byte_width, uacpi_u64 *value) {
    uint16_t p = addr;
    switch (byte_width) {
    case 1: {
        uint8_t v;
        asm ("inb %1, %0" : "=a" (v) : "d" (p));
        *value = v;
        break;
    } case 2: {
        uint16_t v;
        asm ("inw %1, %0" : "=a" (v) : "d" (p));
        *value = v;
        break;
    } case 4: {
        uint32_t v;
        asm ("inl %1, %0" : "=a" (v) : "d" (p));
        *value = v;
        break;
    }
    }
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_raw_io_write(uacpi_io_addr addr, uacpi_u8 byte_width, uacpi_u64 value) {
    uint16_t p = addr;
    switch (byte_width) {
    case 1: {
        uint8_t v = value;
        asm ("outb %0, %1" : : "a" (v), "d" (p));
        break;
    } case 2: {
        uint16_t v = value;
        asm ("outw %0, %1" : : "a" (v), "d" (p));
        break;
    } case 4: {
        uint32_t v = value;
        asm ("outl %0, %1" : : "a" (v), "d" (p));
        break;
    }
    }
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_map(uacpi_io_addr base, uacpi_size, uacpi_handle *out) {
    *out = reinterpret_cast<uacpi_handle>(base);
    return UACPI_STATUS_OK;
}

void uacpi_kernel_io_unmap(uacpi_handle) {}

uacpi_status uacpi_kernel_io_read(uacpi_handle handle, uacpi_size off, uacpi_u8 byte_width, uacpi_u64 *val) {
    return uacpi_kernel_raw_io_read(reinterpret_cast<uacpi_io_addr>(handle) + off, byte_width, val);
}

uacpi_status uacpi_kernel_io_write(uacpi_handle handle, uacpi_size off, uacpi_u8 byte_width, uacpi_u64 val) {
    return uacpi_kernel_raw_io_write(reinterpret_cast<uacpi_io_addr>(handle) + off, byte_width, val);
}

uacpi_status uacpi_kernel_pci_read(uacpi_pci_address *addr, uacpi_size off, uacpi_u8 bw, uacpi_u64 *val) {
    assert(addr->segment == 0);
    switch (bw) {
    case 1:
        *val = pci::read<uint8_t>(addr->bus, addr->device, addr->function, off);
        break;
    case 2:
        *val = pci::read<uint16_t>(addr->bus, addr->device, addr->function, off);
        break;
    case 4:
        *val = pci::read<uint32_t>(addr->bus, addr->device, addr->function, off);
        break;
    }
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_pci_write(uacpi_pci_address *addr, uacpi_size off, uacpi_u8 bw, uacpi_u64 val) {
    assert(addr->segment == 0);
    switch (bw) {
    case 1:
        pci::write<uint8_t>(addr->bus, addr->device, addr->function, off, val);
        break;
    case 2:
        pci::write<uint16_t>(addr->bus, addr->device, addr->function, off, val);
        break;
    case 4:
        pci::write<uint32_t>(addr->bus, addr->device, addr->function, off, val);
        break;
    }
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_schedule_work(uacpi_work_type work_type, uacpi_work_handler handler, uacpi_handle handle) {
    auto p = new proc::process{reinterpret_cast<uintptr_t>(handler), true};
    p->ef.rdi = reinterpret_cast<uintptr_t>(handle);
    if (work_type == UACPI_WORK_GPE_EXECUTION)
        p->cpu = 0;
    p->enqueue();
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_wait_for_work_completion() {
    return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_thread_id uacpi_kernel_get_thread_id() {
    return reinterpret_cast<void*>(syscall(SYS_getpid));
}