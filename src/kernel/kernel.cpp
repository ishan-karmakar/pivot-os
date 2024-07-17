#include <init.hpp>
#include <io/serial.hpp>
#include <cpu/gdt.hpp>
#include <cpu/idt.hpp>
#include <mem/pmm.hpp>
#include <mem/mapper.hpp>
#include <drivers/framebuffer.hpp>
#include <mem/vmm.hpp>
#include <mem/heap.hpp>
#include <acpi/acpi.hpp>
#include <acpi/madt.hpp>
#include <libc/string.h>
#include <cpu/tss.hpp>
#include <cpu/lapic.hpp>
#include <cpu/ioapic.hpp>
#include <stdio.h>
#include <common.h>
#define STACK_CHK_GUARD 0x595e9fbd94fda766
#define EH_OBJECT_SIZE 48

extern char __start_eh_frame;
extern "C" void __register_frame(const void*);

uint8_t CPU = 0;
cpu::GDT::gdt_desc initial_gdt[3];
uintptr_t __stack_chk_guard = STACK_CHK_GUARD;
FILE *stderr = { 0 };

cpu::GDT init_sgdt();
cpu::GDT init_hgdt(cpu::GDT&, mem::Heap&);

extern void io_char_printer(unsigned char);

extern "C" void __attribute__((noreturn)) init_kernel(boot_info *bi) {
    char_printer = io_char_printer;
    io::SerialPort qemu{0x3F8};
    qemu.set_global();

    cpu::GDT sgdt{init_sgdt()};
    sgdt.load();

    cpu::IDT idt;
    cpu::load_exceptions(idt);
    idt.load();

    mem::PMM::init(bi);
    mem::PTMapper mapper{bi->pml4};
    drivers::Framebuffer fb{bi, mapper};
    mem::VMM vmm{mem::VMM::Supervisor, bi->mem_pages, mapper};
    mem::Heap heap{vmm, HEAP_SIZE};
    mem::kheap = &heap;

    __register_frame(&__start_eh_frame);
    call_constructors();
    cpu::GDT hgdt{init_hgdt(sgdt, heap)};
    // hgdt.load();
    // cpu::TSS tss{hgdt, heap};
    // tss.set_rsp0();
    // acpi::ACPI::init(bi->rsdp);
    // cpu::LAPIC::init(mapper, idt);
    // cpu::IOAPIC::init(mapper);
    // cpu::LAPIC::calibrate(ioapic);
    while(1);
}

cpu::GDT init_sgdt() {
    cpu::GDT sgdt{initial_gdt};
    sgdt.set_entry(1, 0b10011011, 0b10);
    sgdt.set_entry(2, 0b10010011, 0);
    return sgdt;
}

class my_exception : std::exception {
    const char *what() const noexcept override { return "test exception"; }
};

cpu::GDT init_hgdt(cpu::GDT& old, mem::Heap& heap) {
    throw my_exception();
    auto madt = acpi::ACPI::get_table<acpi::MADT>();
    size_t num_cpus = 0;
    if (!madt.has_value())
        log(Warning, "ACPI", "Could not find MADT");
    for (auto iter = madt.value().iter<acpi::MADT::lapic>(); iter; ++iter, num_cpus++);
    log(Info, "KERNEL", "Number of CPUs: %u", num_cpus);
    auto heap_gdt = reinterpret_cast<cpu::GDT::gdt_desc*>(heap.alloc((5 + num_cpus * 2) * sizeof(cpu::GDT::gdt_desc)));
    cpu::GDT gdt{heap_gdt};
    gdt = old;
    return gdt;
}

[[noreturn]]
extern "C" void __stack_chk_fail() {
    log(Error, "KERNEL", "Detected stack smashing");
    abort();
}

extern "C" void abort() {
    log(Error, "KERNEL", "Aborting code");
    while(1);
}

extern "C" int pthread_mutex_lock(void*) {
    return 0;
}

extern "C" int pthread_mutex_unlock(void*) {
    return 0;
}

extern "C" int dl_iterate_phdr() {
    return -1;
}

int add_string(char *buf, const char *c) {
    size_t i = 0;
    for (; *c != '\0'; c++)
        buf[i++] = *c;
    return i;
}

int vsprintf(char *buf, const char *fmt, va_list args) {
    log(Info, "SPRINTF", "VSPRINTF called - '%s'", fmt);
    size_t i = 0;
    for (; *fmt != '\0'; fmt++) {
        if (*fmt != '%' || (*fmt == '%' && *(fmt + 1) == '%'))
            buf[i++] = *fmt;
        else {
            switch (*++fmt) {
                case 's': {
                    i += add_string(buf, va_arg(args, char*));
                    break;
                } case 'c': {
                    char ch = (char) va_arg(args, int);
                    buf[i++] = ch;
                    break;
                } case 'd': {
                    i += itoa(va_arg(args, int64_t), buf, 10);
                    break;
                } case 'u': {
                    i += ultoa(va_arg(args, uint64_t), buf, 10);
                    break;
                } case 'x': {
                    add_string(buf, "0x");
                    i += ultoa(va_arg(args, uint64_t), buf, 16);
                    break;
                } case 'b': {
                    add_string(buf, "0b");
                    i += ultoa(va_arg(args, uint64_t), buf, 2);
                } default: {
                    // printf("\n");
                    log(Warning, "STDIO", "Unrecognized identifier %%%c", *fmt++);
                }
            }
        }
    }
    buf[i] = 0;
    return i;
}

extern "C" int __sprintf_chk(char* s, int, size_t slen, const char* format, ...)
{
    va_list ap;

    if (slen == 0)
        abort();

    va_start(ap, format);
    int ret = vsprintf(s, format, ap);
    va_end(ap);

    return ret;
}

int fputc(int ch, FILE *stream) {
    if (stream != stderr)
        log(Warning, "FPUTC", "FPUTC received stream != stderr");
    char_printer(ch);
    return ch;
}

int fputs(const char *str, FILE *stream) {
    if (stream != stderr)
        log(Warning, "FPUTS", "FPUTS received stream != stderr");
    for (; *str; str++)
        fputc(*str, stream);
    return 0;
}

size_t fwrite(const void *buffer, size_t size, size_t count, FILE *stream) {
    if (stream != stderr)
        log(Warning, "FWRITE", "FWRITE received stream != stderr");
    
    const unsigned char *buf = reinterpret_cast<const unsigned char*>(buffer);
    for (size_t i = 0; i < count; i++)
        for (size_t j = 0; j < size; j++)
            fputc(*buf++, stream);
    return count;
}

extern "C" ssize_t write(int fd, const void *buf, size_t count) {
    log(Info, "WRITE", "WRITE called with fd %u, buf address %x, and count %u", fd, buf, count);
    return count;
}