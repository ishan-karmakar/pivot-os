#include <loader.h>
#include <uefi.h>
#include <common.h>
#include <mem.h>
#include <util/logger.h>
#include <libc/string.h>
#define KERNEL_PATH L"\\kernel.elf"

efi_guid_t sfsp_guid = { 0x964e5b22, 0x6459, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b} };

efi_status_t load_segment(elf64_phdr_t *phdr, efi_file_handle_t *kernel) {
    efi_status_t status;
    void *data;
    size_t buffer_read_size = phdr->p_filesz;
    uint16_t page_offset = phdr->p_vaddr % PAGE_SIZE;
    size_t num_pages = DIV_CEIL(phdr->p_memsz + page_offset, PAGE_SIZE);
    status = kernel->set_pos(kernel, phdr->p_offset);
    if (status < 0) return status;

    status = gST->boot_services->alloc_pages(AllocateAnyPages, EfiLoaderCode, num_pages, &data);
    if (status < 0) return status;

    status = kernel->read(kernel, &buffer_read_size, data + page_offset);
    if (status < 0) return status;

    gST->boot_services->set_mem(data + page_offset + buffer_read_size, phdr->p_memsz - buffer_read_size, 0);

    for (size_t i = 0; i < num_pages; i++)
        map_addr(ALIGN_ADDR((uintptr_t) data) + PAGE_SIZE * i, ALIGN_ADDR(phdr->p_vaddr) + PAGE_SIZE * i, KERNEL_PT_ENTRY, gBI.pml4);
    return 0;
}

efi_status_t load_kernel(uintptr_t *entry) {
    efi_status_t status;
    size_t buffer_read_size;

    efi_sfsp_t *sfsp;
    status = gST->boot_services->locate_protocol(&sfsp_guid, NULL, &sfsp);
    if (status < 0) return status;

    efi_file_handle_t *root_fs;
    status = sfsp->open_volume(sfsp, &root_fs);
    if (status < 0) return status;

    efi_file_handle_t *kernel;
    status = root_fs->open(root_fs, &kernel, KERNEL_PATH, 1, 1);
    if (status < 0) return status;

    status = kernel->set_pos(kernel, 0);
    if (status < 0) return status;

    buffer_read_size = sizeof(elf64_ehdr_t);
    elf64_ehdr_t ehdr;
    status = gST->boot_services->alloc_pool(EfiLoaderData, buffer_read_size, &ehdr);
    if (status < 0) return status;

    status = kernel->read(kernel, &buffer_read_size, &ehdr);
    if (status < 0) return status;

    char elf_ident[4] = { 0x7F, 'E', 'L', 'F' };
    if (memcmp(ehdr.e_ident, elf_ident, 4))
        return -2;
    
    log(Verbose, "LOADER", "ELF file verified");

    if (ehdr.e_ident[4] == 1)
        return -3;
    
    buffer_read_size = sizeof(elf64_phdr_t) * ehdr.e_phnum;
    status = kernel->set_pos(kernel, ehdr.e_phoff);
    if (status < 0) return status;

    elf64_phdr_t *phdrs;
    status = gST->boot_services->alloc_pool(EfiLoaderData, buffer_read_size, &phdrs);
    if (status < 0) return status;

    status = kernel->read(kernel, &buffer_read_size, phdrs);
    if (status < 0) return status;

    // uint64_t load_segments = 0;
    // for (uint16_t i = 0; i < ehdr.e_phnum; i++)
    //     load_segments += phdrs[i].p_type == 1;

    for (uint16_t i = 0; i < ehdr.e_phnum; i++)
        if (phdrs[i].p_type == 1) {
            status = load_segment(phdrs + i, kernel);
            if (status < 0) return status;
        }
    log(Verbose, "LOADER", "Loaded program segments");

    status = gST->boot_services->free_pool(phdrs);
    if (status < 0) return status;

    return 0;
}