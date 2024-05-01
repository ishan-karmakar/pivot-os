#include <mem/vmm.h>
#include <mem/pmm.h>
#include <sys.h>
#include <kernel/logging.h>

vmm_info_t vmm_kernel;

// Implementation taken from Dreamos64
void init_vmm(vmm_level_t level, vmm_info_t *vmm_info) {
    if (vmm_info == NULL) {
        vmm_info = &vmm_kernel;
        vmm_info->p4_tbl = NULL; // Make sure to initialize kernel VMM before calling ANY valloc
    }

    vmm_info->flags = level == Supervisor ? KERNEL_PT_ENTRY : USER_PT_ENTRY;
    vmm_info->data_start = HIGHER_HALF_OFFSET + (mem_info->mem_pages + 1) * PAGE_SIZE;
    vmm_info->status.root_container = (vmm_container_t*) vmm_info->data_start;

    if (level == Supervisor)
        vmm_info->space_start = vmm_info->data_start + VMM_RESERVED_SPACE_SIZE + PAGE_SIZE;
    else
        vmm_info->space_start = PAGE_SIZE; // Padding to make sure *0 isn't valid
    
    vmm_info->status.next_addr = vmm_info->space_start;
    vmm_info->status.cur_index = 0;
    
    void *vmm_root = alloc_frame();
    if (vmm_root == NULL)
        return log(Error, "VMM", "Cannot allocate frame for start of VMM space");

    map_addr((uintptr_t) vmm_root, vmm_info->data_start, vmm_info->flags, vmm_info->p4_tbl);

    vmm_info->status.root_container->next = NULL;
    vmm_info->status.cur_container = vmm_info->status.root_container;
    log(Info, "VMM", "Initialized virtual memory manager");
}

void *valloc(size_t size, size_t flags, vmm_info_t *vmm_info) {
    if (size == 0)
        return NULL;
    
    if (vmm_info == NULL)
        vmm_info = &vmm_kernel;
    if (vmm_info->status.cur_index >= VMM_ITEMS_PER_PAGE) {
        void *container_phys_addr = alloc_frame();
        if (!container_phys_addr) {
            log(Error, "VMM", "Couldn't allocate a new page for a new container");
            return NULL;
        }
        vmm_container_t *new_container = (vmm_container_t*) ALIGN_ADDR_UP((uintptr_t) vmm_info->status.cur_container + sizeof(vmm_container_t));
        map_addr((uintptr_t) container_phys_addr, (uintptr_t) new_container, vmm_info->flags, vmm_info->p4_tbl);
        vmm_info->status.cur_index = 0;
        new_container->next = NULL;
        vmm_info->status.cur_container->next = new_container;
        vmm_info->status.cur_container = new_container;
    }

    size_t num_pages = ALIGN_ADDR_UP(size) / PAGE_SIZE;
    uintptr_t addr = vmm_info->status.next_addr;
    vmm_item_t *item = vmm_info->status.cur_container->items + vmm_info->status.cur_index;
    item->base = addr;
    item->flags = flags;
    item->size = num_pages;
    vmm_info->status.next_addr += num_pages * PAGE_SIZE;
    vmm_info->status.cur_index++;
    for (size_t i = 0; i < num_pages; i++) {
        uintptr_t f = (uintptr_t) alloc_frame();
        map_addr(f, addr + i * PAGE_SIZE, flags | vmm_info->flags, vmm_info->p4_tbl);
    }

    return (void*) addr;
}

// TODO: Implement this function
void vfree(__attribute__((unused)) void *addr) {
}
