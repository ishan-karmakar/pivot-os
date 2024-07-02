set(CMAKE_C_COMPILER clang)
cmake_minimum_required(VERSION 3.5)
project(PivotOS C ASM_NASM)
file(GLOB_RECURSE COMMON_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/common/*.c")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -ffreestanding -fno-stack-protector -nostdlib -mno-red-zone -mno-sse")
set(QEMU_FLAGS -m 128M -smp 2 -enable-kvm -serial stdio -bios OVMF.fd -no-reboot -no-shutdown)
include_directories(include/common)
find_program(QEMU_PATH qemu-system-x86_64)
add_subdirectory(src/boot boot)
add_subdirectory(src/kernel kernel)
add_custom_target(BuildImage ALL DEPENDS os.img)
add_custom_command(
    OUTPUT os.img
    COMMAND ${CMAKE_SOURCE_DIR}/efi2img.sh boot/BOOTX64.efi kernel/kernel.elf os.img
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS BOOTX64.efi kernel.elf
)

if (QEMU_PATH)
    add_custom_target(
        run
        COMMAND ${QEMU_PATH} ${QEMU_FLAGS} -drive file=${CMAKE_CURRENT_BINARY_DIR}/os.img,index=0,media=disk,format=raw
        DEPENDS os.img
        VERBATIM
    )
endif ()
