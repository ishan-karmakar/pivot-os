#include <boot/loader.h>
#include <boot/mem.h>
#include <sys.h>
#define KERNEL_PATH L"\\kernel.elf"

EFI_STATUS LoadSegment(elf64_phdr_t *program_header, EFI_FILE *kernel_file, boot_info_t *boot_info, EFI_PHYSICAL_ADDRESS *kernel_entries_location, UINTN *total_num_pages) {
    EFI_STATUS status;
    VOID *segment_data = NULL;
    UINTN buffer_read_size = program_header->p_filesz;
    UINTN page_offset = program_header->p_vaddr % EFI_PAGE_SIZE;
    UINTN num_pages = EFI_SIZE_TO_PAGES(page_offset + program_header->p_memsz);
    *total_num_pages += num_pages;
    Print(L"Number of pages needed for segment: %u\n", num_pages);
    status = uefi_call_wrapper(kernel_file->SetPosition, 2, kernel_file, program_header->p_offset);
    if (EFI_ERROR(status)) {
        Print(L"Error setting position of file\n");
        return status;
    }

    status = uefi_call_wrapper(gBS->AllocatePages, 4, AllocateAnyPages, EfiLoaderCode, num_pages, &segment_data);
    if (EFI_ERROR(status)) {
        Print(L"Error allocating pages for segment\n");
        return status;
    }
    for (UINTN i = 0; i < num_pages; i++) {
        kernel_entries_location[i] = (EFI_PHYSICAL_ADDRESS) segment_data + PAGE_SIZE * i;
    }

    status = uefi_call_wrapper(kernel_file->Read, 3, kernel_file, &buffer_read_size, segment_data);
    if (EFI_ERROR(status)) {
        Print(L"Error reading segment data from file\n");
        return status;
    }

    status = uefi_call_wrapper(gBS->SetMem, 3, segment_data + buffer_read_size, program_header->p_memsz - buffer_read_size, 0);
    if (EFI_ERROR(status)) {
        Print(L"Error zero filling segment\n");
        return status;
    }
    for (UINTN i = 0; i < num_pages; i++)
        MapAddr(ALIGN_ADDR(program_header->p_vaddr + EFI_PAGE_SIZE * i), ALIGN_ADDR((EFI_PHYSICAL_ADDRESS) (segment_data + EFI_PAGE_SIZE * i)), boot_info->pml4);
    return EFI_SUCCESS;
}

EFI_STATUS LoadKernel(boot_info_t *boot_info, EFI_PHYSICAL_ADDRESS *kernel_entry_point) {
    EFI_STATUS status;
    EFI_FILE *kernel_file;
    UINTN buffer_read_size;
    UINT8 *elf_header_buf = NULL;
    UINT8 *elf_pheader_buf = NULL;
    UINT16 num_program_segments;
    UINTN program_headers_offset;

    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fsp;
    status = uefi_call_wrapper(gBS->LocateProtocol, 3, &gEfiSimpleFileSystemProtocolGuid, NULL, &fsp);
    if (EFI_ERROR(status)) {
        Print(L"Error locating Simple File System Protocol\n");
        return status;
    }
    Print(L"Located Simple File System Protocol...\n");

    EFI_FILE *root_fs;
    status = uefi_call_wrapper(fsp->OpenVolume, 2, fsp, &root_fs);
    if (EFI_ERROR(status)) {
        Print(L"Error opening root volume\n");
        return status;
    }
    Print(L"Opened root volume...\n");


    status = uefi_call_wrapper(root_fs->Open, 5, root_fs, &kernel_file, KERNEL_PATH, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
    if (EFI_ERROR(status)) {
        Print(L"Error opening kernel file\n");
        return status;
    }

    status = uefi_call_wrapper(kernel_file->SetPosition, 2, kernel_file, 0);
    if (EFI_ERROR(status)) {
        Print(L"Error setting position of file\n");
        return status;
    }

    buffer_read_size = sizeof(elf64_ehdr_t);
    
    status = uefi_call_wrapper(kernel_file->SetPosition, 2, kernel_file, 0);
    if (EFI_ERROR(status)) {
        Print(L"Error setting position of file\n");
        return status;
    }

    status = uefi_call_wrapper(gBS->AllocatePool, 3, EfiLoaderData, buffer_read_size, &elf_header_buf);
    if (EFI_ERROR(status)) {
        Print(L"Error allocating kernel header buffer\n");
        return status;
    }

    status = uefi_call_wrapper(kernel_file->Read, 3, kernel_file, &buffer_read_size, elf_header_buf);
    if (EFI_ERROR(status)) {
        Print(L"Error reading from file\n");
        return status;
    }

    if (elf_header_buf[0] != 0x7F ||
        elf_header_buf[1] != 0x45 ||
        elf_header_buf[2] != 0x4C ||
        elf_header_buf[3] != 0x46) {
        Print(L"Invalid ELF header\n");
        return EFI_INVALID_PARAMETER;
    }

    UINT8 file_type = elf_header_buf[4];
    Print(L"Kernel file type: %u\n", file_type);

    if (file_type == 1) {
        Print(L"32bit ELF Files are not supported\n");
        return EFI_UNSUPPORTED;
    }

    elf64_ehdr_t *hdr = (elf64_ehdr_t*) elf_header_buf;
    program_headers_offset = hdr->e_phoff;
    num_program_segments = hdr->e_phnum;
    buffer_read_size = sizeof(elf64_phdr_t) * hdr->e_phnum;
    *kernel_entry_point = hdr->e_entry;

    status = uefi_call_wrapper(kernel_file->SetPosition, 2, kernel_file, program_headers_offset);
    if (EFI_ERROR(status)) {
        Print(L"Error setting position of file\n");
        return status;
    }

    status = uefi_call_wrapper(gBS->AllocatePool, 3, EfiLoaderData, buffer_read_size, &elf_pheader_buf);
    if (EFI_ERROR(status)) {
        Print(L"Error allocating ELF Program Header buffer\n");
        return status;
    }

    status = uefi_call_wrapper(kernel_file->Read, 3, kernel_file, &buffer_read_size, elf_pheader_buf);
    if (EFI_ERROR(status)) {
        Print(L"Error reading ELF Program Headers from file\n");
        return status;
    }

    UINTN total_num_pages = 0;
    elf64_phdr_t *program_headers = (elf64_phdr_t*) elf_pheader_buf;
    for (UINT16 p = 0; p < num_program_segments; p++) {
        if (program_headers[p].p_type != 1) continue;
        UINTN page_offset = program_headers[p].p_vaddr % EFI_PAGE_SIZE;
        UINTN num_pages = EFI_SIZE_TO_PAGES(page_offset + program_headers[p].p_memsz);
        total_num_pages += num_pages;
    }
    Print(L"Total num pages: %u\n", total_num_pages);

    UINT64 *kernel_entries_location;
    status = uefi_call_wrapper(gBS->AllocatePool, 3, EfiLoaderData, total_num_pages * 4, &kernel_entries_location);
    if (EFI_ERROR(status)) {
        Print(L"Error allocating pool to store kernel entries\n");
        return status;
    }

    UINTN idx = 0;
    for (UINT16 p = 0; p < num_program_segments; p++) {
        if (program_headers[p].p_type != 1) continue;
        status = LoadSegment(program_headers + p, kernel_file, boot_info, kernel_entries_location + idx, &idx);
        if (EFI_ERROR(status))
            return status;
    }
    Print(L"Loaded program segments...\n");

    status = uefi_call_wrapper(kernel_file->Close, 1, kernel_file);
    if (EFI_ERROR(status)) {
        Print(L"Error closing kernel file\n");
        return status;
    }


    status = uefi_call_wrapper(gBS->FreePool, 1, elf_header_buf);
    if (EFI_ERROR(status)) {
        Print(L"Error freeing kernel header buffer\n");
        return status;
    }

    status = uefi_call_wrapper(gBS->FreePool, 1, elf_pheader_buf);
    if (EFI_ERROR(status)) {
        Print(L"Error freeing kernel program header buffer\n");
        return status;
    }

    boot_info->kernel_entries = kernel_entries_location;
    boot_info->num_kernel_entries = total_num_pages;

    return EFI_SUCCESS;
}