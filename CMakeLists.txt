cmake_minimum_required(VERSION 3.10)
project(PivotOS NONE)
file(GLOB_RECURSE COMMON_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/common/*.c")
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -ffreestanding -fno-stack-protector -nostdlib -mno-red-zone")
set(QEMU_FLAGS -m 128M -smp 2 -enable-kvm -serial stdio -no-reboot -no-shutdown)
set(OS_IMG ${CMAKE_CURRENT_BINARY_DIR}/os.img)
set(BIOS1_BIN ${CMAKE_CURRENT_BINARY_DIR}/bios/bios1.bin)
set(BIOS2_BIN ${CMAKE_CURRENT_BINARY_DIR}/bios/bios2.bin)
include_directories(include/common)
find_program(SGDISK sgdisk REQUIRED)
find_program(FAT_MKFS mkfs.fat REQUIRED)
find_program(MMD mmd REQUIRED)
find_program(MCOPY mcopy REQUIRED)

configure_file(dmconfig dmconfig)
file(GENERATE OUTPUT dmconfig INPUT ${CMAKE_CURRENT_BINARY_DIR}/dmconfig)

add_subdirectory(src/diskmaker diskmaker)
add_subdirectory(src/uefi uefi)
add_subdirectory(src/bios bios)
add_subdirectory(src/kernel kernel)

add_custom_command(
    OUTPUT ${OS_IMG}
    COMMAND $<TARGET_FILE:diskmaker>
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    BYPRODUCTS tmp.img
    DEPENDS diskmaker BOOTX64.efi kernel.elf ${BIOS1_BIN} ${BIOS2_BIN}
)
    
add_custom_target(os ALL DEPENDS ${OS_IMG} diskmaker BOOTX64.efi kernel.elf bios1.bin bios2.bin)

find_program(QEMU_PATH qemu-system-x86_64)
if (NOT QEMU_BIOS)
    list(APPEND QEMU_FLAGS -bios OVMF.fd)
endif()

if (QEMU_PATH AND NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    add_custom_target(
        run
        COMMAND ${QEMU_PATH} ${QEMU_FLAGS} -drive file=${OS_IMG},index=0,media=disk,format=raw
        DEPENDS os
    )
endif ()
