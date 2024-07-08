#include <loader.h>
#include <uefi.h>
#include <util/logger.h>
#include <string.h>
#define KERNEL_PATH L"\\kernel.elf"

efi_guid_t sfsp_guid = { 0x964e5b22, 0x6459, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b} };

efi_status_t load_kernel(uintptr_t *entry) {
    efi_status_t status;
    size_t buffer_read_size;

    efi_sfsp_t *sfsp;
    status = gST->boot_services->locate_protocol(&sfsp_guid, NULL, &sfsp);
    if (status < 0) return status;
    log(Verbose, "LOADER", "Opened Simple File System Protocol");

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
}