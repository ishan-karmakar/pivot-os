#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <boot.h>
#include <mem/vmm.h>
#define KMEM kinfo.mem
#define KFB kinfo.fb
#define KACPI kinfo.acpi
#define KLAPIC kinfo.lapic
#define KIOAPIC kinfo.ioapic
#define KVMM kinfo.vmm
#define KHEAP kinfo.heap
#define KIPC kinfo.ipc
#define KSMP kinfo.smp
#define KCPUS kinfo.smp.cpus

#define KERNEL_VIRTUAL_ADDR 0xFFFFFFFF80000000
#define ALIGN_ADDR_UP(address) ALIGN_ADDR((address) + (PAGE_SIZE - 1))

typedef struct kernel_entry {
    uintptr_t vaddr;
    uintptr_t paddr;
    size_t num_pages;
} kernel_entry_t;

typedef struct cpu_data {
    struct thread * volatile threads;
    struct thread *wakeups;
    struct thread *cur;
    size_t num_threads;
    volatile size_t ticks;
    volatile bool trig;
    uintptr_t stack; // Stack bottom
} cpu_data_t;

typedef struct kernel_info {
    struct {
        mmap_desc_t *mmap;
        uint64_t mmap_size;
        uint64_t mmap_desc_size;
        size_t num_ke;
        kernel_entry_t *ke;
        uint64_t *pml4;
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
        bool x2mode;
        size_t ms_interval;
        volatile size_t pit_ticks;
    } lapic;

    struct {
        uintptr_t addr;
        uint32_t gsi_base;
        size_t num_ovrds;
        struct ioapic_so *ovrds;
    } ioapic;

    volatile struct {
        uint8_t action;
        uintptr_t addr;
    } ipc;

    struct {
        struct thread *idle;
        size_t num_cpus;
        cpu_data_t *cpus;
    } smp;
    vmm_t vmm;
    struct heap *heap;
} kernel_info_t;

extern struct kernel_info kinfo;
extern uint8_t CPU;
