project(bios ASM_NASM)

add_custom_target(bios1.bin DEPENDS ${BIOS1_BIN})
add_custom_command(
    OUTPUT ${BIOS1_BIN}
    COMMAND ${CMAKE_ASM_NASM_COMPILER} -o ${BIOS1_BIN} boot1.asm
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS boot1.asm screen16.asm
)

add_custom_target(bios2.bin DEPENDS ${BIOS2_BIN})
add_custom_command(
    OUTPUT ${BIOS2_BIN}
    COMMAND ${CMAKE_ASM_NASM_COMPILER} -o ${BIOS2_BIN} boot2.asm
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS boot2.asm screen32.asm
)