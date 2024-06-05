#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <mem/pmm.h>
#include <mem/vmm.h>
#include <mem/heap.h>
#include <cpu/ioapic.h>
#include <scheduler/thread.h>
#define KMEM kinfo.mem
#define KFB kinfo.fb
#define KACPI kinfo.acpi
#define KLAPIC kinfo.lapic
#define KIOAPIC kinfo.ioapic
#define KVMM kinfo.vmm
#define KHEAP kinfo.heap
#define KSCHED kinfo.scheduler

#define MIN_LOG_LEVEL Verbose
#define PAGE_SIZE 4096
#define KERNEL_VIRTUAL_ADDR 0xFFFFFFFF80000000
#define HIGHER_HALF_OFFSET 0xFFFF800000000000
#define SIZE_TO_PAGES(size) DIV_CEIL(size, PAGE_SIZE)
#define VADDR(addr) (((uintptr_t) (addr)) | HIGHER_HALF_OFFSET)
#define PADDR(addr) (((uintptr_t) (addr) & ~HIGHER_HALF_OFFSET))
#define ALIGN_ADDR(address) ((address) & -PAGE_SIZE)
#define ALIGN_ADDR_UP(address) ALIGN_ADDR((address) + (PAGE_SIZE - 1))
#define DIV_CEIL(num, dividend) (((num) + ((dividend) - 1)) / (dividend))
#define P4_ENTRY(addr) (((addr) >> 39) & 0x1FF)
#define P3_ENTRY(addr) (((addr) >> 30) & 0x1FF)
#define P2_ENTRY(addr) (((addr) >> 21) & 0x1FF)
#define P1_ENTRY(addr) (((addr) >> 12) & 0x1FF)
#define SIGN_MASK 0x000ffffffffff000
#define KERNEL_PT_ENTRY 0b11
#define USER_PT_ENTRY 0b111

typedef struct mmap_descriptor {
    uint32_t type;
    uint32_t pad;
    uintptr_t physical_start;
    uintptr_t virtual_start;
    uint64_t count;
    uint64_t attributes;
} mmap_desc_t;

typedef struct kernel_entry {
    uintptr_t vaddr;
    uintptr_t paddr;
    size_t num_pages;
} kernel_entry_t;

typedef struct kernel_info {
    struct {
        mmap_desc_t *mmap;
        uint64_t mmap_size;
        uint64_t mmap_desc_size;
        size_t num_ke;
        kernel_entry_t *ke;
        page_table_t pml4;
        uint64_t *bitmap;
        size_t bitmap_entries;
        size_t bitmap_size;
        size_t mem_pages;
    } mem;

    struct {
        char *buffer;
        uint32_t horizontal_res;
        uint32_t vertical_res;
        uint32_t pixels_per_scanline;
        uint8_t bpp;
    } fb;

    struct {
        uintptr_t sdt_addr;
        bool xsdt;
        size_t num_tables;
    } acpi;

    struct {
        uintptr_t addr;
        int8_t x2mode; // -1 for no x2apic, 1 for yes x2apic, and 0 for uninitialized
        size_t ms_interval;
        volatile size_t pit_ticks;
        volatile size_t apic_ticks;
        volatile bool oneshot_trig;
    } lapic;

    struct {
        uintptr_t addr;
        uint32_t gsi_base;
        size_t num_ovrds;
        ioapic_so_t *ovrds;
    } ioapic;

    struct {
        thread_t *threads;
        thread_t *wakeups;
        thread_t *idle;
        thread_t *cur;
    } scheduler;

    vmm_t vmm;
    heap_t *heap;
} kernel_info_t;

extern struct kernel_info kinfo;
