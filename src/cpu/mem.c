#include <stddef.h>
#include <cpu/mem.h>
#include <kernel/multiboot.h>
#include <kernel/logging.h>

void initialize_bitmap(uintptr_t free_area, size_t mem_size) {
    uint32_t bitmap_size = mem_size / PAGE_SIZE + 1;
    uint32_t used_frames = 0;
    
}

void init_mem(uintptr_t addr, uint32_t size, size_t mem_size) {
    initialize_bitmap(addr + size, mem_size);
    uint64_t bitmap_start_addr;
    size_t bitmap_size;
    bitmap_get_region(&bitmap_start_addr, &bitmap_size, ADDRESS_TYPE_PHYSICAL);
}
