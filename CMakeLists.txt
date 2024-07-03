set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
cmake_minimum_required(VERSION 3.10)
project(PivotOS)
file(GLOB_RECURSE COMMON_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/common/*.c")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -ffreestanding -fno-stack-protector -nostdlib -mno-red-zone -mno-sse")
set(QEMU_FLAGS -m 128M -smp 2 -enable-kvm -serial stdio -no-reboot -no-shutdown)
set(OS_IMG ${CMAKE_CURRENT_BINARY_DIR}/os.img)
include_directories(include/common)
find_program(SGDISK sgdisk REQUIRED)
find_program(FAT_MKFS mkfs.fat REQUIRED)
find_program(MMD mmd REQUIRED)
find_program(MCOPY mcopy REQUIRED)

configure_file(diskmaker.sh.in diskmaker.sh @ONLY)
file(GENERATE OUTPUT diskmaker.sh INPUT ${CMAKE_CURRENT_BINARY_DIR}/diskmaker.sh FILE_PERMISSIONS OWNER_READ OWNER_EXECUTE)

add_subdirectory(src/uefi uefi)
add_subdirectory(src/bios bios)
add_subdirectory(src/kernel kernel)

add_custom_command(
    OUTPUT ${OS_IMG}
    COMMAND ./diskmaker.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    BYPRODUCTS tmp.img
    DEPENDS BOOTX64.efi kernel.elf bios.bin
)
    
add_custom_target(os ALL DEPENDS ${OS_IMG})

find_program(QEMU_PATH qemu-system-x86_64)
if (NOT QEMU_BIOS)
    list(APPEND QEMU_FLAGS -bios OVMF.fd)
endif()
unset(QEMU_BIOS CACHE)

if (QEMU_PATH)
    add_custom_target(
        run
        COMMAND ${QEMU_PATH} ${QEMU_FLAGS} -drive file=${OS_IMG},index=0,media=disk,format=raw
        DEPENDS ${OS_IMG}
    )
endif ()
