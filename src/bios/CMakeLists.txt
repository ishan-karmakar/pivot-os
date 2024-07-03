project(bios ASM_NASM)
set(BOOTLOADER_ASM ${CMAKE_CURRENT_SOURCE_DIR}/bootloader.asm)

add_custom_target(
    bios.bin
    DEPENDS ${BIOS_BIN}
)

add_custom_command(
    OUTPUT ${BIOS_BIN}
    COMMAND ${CMAKE_ASM_NASM_COMPILER} ${CMAKE_ASM_NASM_FLAGS} -f bin -o ${BIOS_BIN} ${BOOTLOADER_ASM}
    DEPENDS ${BOOTLOADER_ASM}
)