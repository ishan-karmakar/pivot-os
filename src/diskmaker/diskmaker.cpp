#include <parted/parted.h>
#include <fstream>
#include <filesystem>
#include <cmath>
#include <iostream>
#include <filesystem>

#define GPT_HEADER_SECTORS 33
#define SEC_SIZE 512

int get_file_size(std::filesystem::path, std::filesystem::path);

int main(int argc, const char **argv) {
    if (argc != 4) {
        std::cerr << "Didn't find 3 arguments (EFI, ELF, OUT)\n";
        return 1;
    }
    std::filesystem::path efi_path(argv[1]), elf_path(argv[2]), out_path(argv[3]);
    int file_size = get_file_size(efi_path, elf_path);
    { std::ofstream file(out_path); }
    std::filesystem::resize_file(out_path, file_size * SEC_SIZE);

    PedDevice *device = ped_device_get(out_path.c_str());
    ped_device_open(device);
    PedDiskType *gpt = ped_disk_type_get("gpt");
    PedDisk *disk = ped_disk_new_fresh(device, gpt);
    std::cout << "Created GPT label\n";
    PedPartition *part = ped_partition_new(
        disk,
        PED_PARTITION_NORMAL,
        ped_file_system_type_get("fat32"),
        GPT_HEADER_SECTORS + 1, file_size - GPT_HEADER_SECTORS - 1
    );
    ped_partition_set_name(part, "EFI");
    ped_disk_add_partition(disk, part, ped_device_get_constraint(device));
    std::cout << "Created EFI partition\n";
    ped_disk_commit_to_dev(disk);
    PedFileSystem *fs = ped_file_system_open(&part->geom);
    std::cout << "Commited all data to disk\n";
    ped_disk_destroy(disk);
    ped_device_close(device);
    ped_device_destroy(device);
    return 0;
}

int get_file_size(std::filesystem::path efi_path, std::filesystem::path elf_path) {
    std::ifstream efi(efi_path), elf(elf_path);

    efi.seekg(0, std::ios_base::end);
    elf.seekg(0, std::ios_base::end);

    int efi_length = efi.tellg();
    int elf_length = elf.tellg();

    efi_length = (efi_length + SEC_SIZE - 1) / SEC_SIZE;
    elf_length = (elf_length + SEC_SIZE - 1) / SEC_SIZE;
    int total_length = 1 + GPT_HEADER_SECTORS * 2 + efi_length + elf_length;
    std::cout << "FAT32 sectors: " << efi_length + elf_length << '\n';
    std::cout << "Total file sectors: " << total_length << '\n';

    return total_length;
}