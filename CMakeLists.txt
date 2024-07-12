cmake_minimum_required(VERSION 3.10)
project(PivotOS NONE)
file(GLOB_RECURSE COMMON_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/common/*.c")
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -ffreestanding -fno-exceptions -fno-stack-protector -nostdlib -mno-red-zone")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -ffreestanding -fno-stack-protector -nostdlib -mno-red-zone -fno-exceptions -fno-rtti")
set(QEMU_FLAGS -m 128M -smp 2 -serial stdio -no-reboot -no-shutdown)
include_directories(include/common)
find_program(SGDISK sgdisk REQUIRED)
find_program(FAT_MKFS mkfs.fat REQUIRED)
find_program(MMD mmd REQUIRED)
find_program(MCOPY mcopy REQUIRED)

configure_file(dmconfig dmconfig)
file(GENERATE OUTPUT dmconfig INPUT ${CMAKE_CURRENT_BINARY_DIR}/dmconfig)

find_program(QEMU_PATH qemu-system-x86_64)
if (NOT QEMU_BIOS)
    list(APPEND QEMU_FLAGS -bios OVMF.fd)
endif()

if (QEMU_PATH AND NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    add_definitions(-DDEBUG)
    add_custom_target(
        run
        COMMAND ${QEMU_PATH} ${QEMU_FLAGS} -drive file=os.img,index=0,media=disk,format=raw
        DEPENDS os
    )
endif ()

add_subdirectory(src/diskmaker diskmaker)
add_subdirectory(src/uefi uefi)
add_subdirectory(src/bios bios)
add_subdirectory(src/kernel kernel)

add_custom_target(os ALL DEPENDS os.img diskmaker uefi kernel bios1 bios2)
add_custom_command(
    OUTPUT os.img
    COMMAND $<TARGET_FILE:diskmaker>
    BYPRODUCTS tmp.img
    DEPENDS diskmaker uefi kernel bios/bios1.bin bios/bios2.bin
)
