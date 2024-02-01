#include <mem/vmm.h>
#include <mem/pmm.h>
#include <sys.h>
#include <kernel/logging.h>

vmm_info_t vmm_kernel;

void init_vmm(__attribute__((unused)) vmm_level_t level) {
    vmm_kernel.data_start = ALIGN_ADDR(HIGHER_HALF_OFFSET + (mem_pages + 1) * PAGE_SIZE);
    vmm_kernel.status.root_container = (vmm_container_t*) vmm_kernel.data_start;
    vmm_kernel.status.vmm_data_end = vmm_kernel.data_start + VMM_RESERVED_SPACE_SIZE;

    vmm_kernel.space_start = vmm_kernel.data_start + VMM_RESERVED_SPACE_SIZE + PAGE_SIZE;
    
    vmm_kernel.status.next_addr = vmm_kernel.space_start;
    vmm_kernel.status.cur_index = 0;
    
    void *vmm_root = alloc_frame();
    if (vmm_root == NULL)
        return log(Error, "VMM", "Cannot allocate frame for start of VMM space");
    
    map_addr((uintptr_t) vmm_root, vmm_kernel.data_start, PAGE_TABLE_ENTRY);

    vmm_kernel.status.root_container->next = NULL;
    vmm_kernel.status.cur_container = vmm_kernel.status.root_container;

    log(Info, "VMM", "Initialized virtual memory manager");
}

void *valloc(size_t size, size_t flags) {
    if (size == 0)
        return NULL;
    
    if (vmm_kernel.status.cur_index >= VMM_ITEMS_PER_PAGE) {
        void *container_phys_addr = alloc_frame();
        if (!container_phys_addr) {
            log(Error, "VMM", "Couldn't allocate a new page for a new container");
            return NULL;
        }
        vmm_container_t *new_container = (vmm_container_t*) ALIGN_ADDR_UP((uintptr_t) vmm_kernel.status.cur_container + sizeof(vmm_container_t));
        map_addr((uintptr_t) container_phys_addr, (uintptr_t) new_container, PAGE_TABLE_ENTRY);
        vmm_kernel.status.cur_index = 0;
        new_container->next = NULL;
        vmm_kernel.status.cur_container->next = new_container;
        vmm_kernel.status.cur_container = new_container;
    }

    size_t num_pages = ALIGN_ADDR_UP(size);

    uintptr_t addr = vmm_kernel.status.next_addr;
    vmm_item_t *item = vmm_kernel.status.cur_container->items + vmm_kernel.status.cur_index;
    item->base = addr;
    item->flags = flags;
    item->size = num_pages;
    vmm_kernel.status.next_addr += num_pages;
    vmm_kernel.status.cur_index++;

    for (size_t i = 0; i < num_pages; i++)
        map_addr((uintptr_t) alloc_frame(), addr + i * PAGE_SIZE, flags | PAGE_TABLE_ENTRY);
    
    return (void*) addr;
}

// TODO: Implement this function
void vmm_free(__attribute__((unused)) void *addr) {
}
