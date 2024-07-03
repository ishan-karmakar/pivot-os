set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
cmake_minimum_required(VERSION 3.10)
project(PivotOS)
file(GLOB_RECURSE COMMON_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/common/*.c")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -ffreestanding -fno-stack-protector -nostdlib -mno-red-zone -mno-sse")
set(QEMU_FLAGS -m 128M -smp 2 -enable-kvm -serial stdio -bios OVMF.fd -no-reboot -no-shutdown)
include_directories(include/common)
find_program(RM rm REQUIRED)
find_program(DU du REQUIRED)
find_program(DD dd REQUIRED)
find_program(SFDISK sfdisk REQUIRED)
find_program(FAT_MKFS mkfs.fat REQUIRED)
find_program(MMD mmd REQUIRED)
find_program(MCOPY mcopy REQUIRED)
configure_file(diskmaker.sh.in diskmaker.sh)
file(GENERATE OUTPUT diskmaker.sh INPUT ${CMAKE_CURRENT_BINARY_DIR}/diskmaker.sh FILE_PERMISSIONS OWNER_READ OWNER_EXECUTE)
# configure_file(diskmaker.sh.in diskmaker.sh @ONLY FILE_PERMISSIONS OWNER_READ OWNER_EXECUTE)
configure_file(disk_layout.txt disk_layout.txt COPYONLY)

add_subdirectory(src/boot boot)
add_subdirectory(src/kernel kernel)

add_custom_target(os ALL DEPENDS os.img)
add_custom_command(
    OUTPUT os.img
    COMMAND ./diskmaker.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS BOOTX64.efi kernel.elf
)

find_program(QEMU_PATH qemu-system-x86_64)
if (QEMU_PATH)
    add_custom_target(
        run
        COMMAND ${QEMU_PATH} ${QEMU_FLAGS} -drive file=$<TARGET_FILE:os.>,index=0,media=disk,format=raw
        DEPENDS os
    )
endif ()
