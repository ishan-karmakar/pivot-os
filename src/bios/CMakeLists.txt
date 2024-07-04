project(bios ASM_NASM)
set(
    BIOS_HELPERS
    util.asm
    util16.asm
    util32.asm
)

add_custom_target(bios1.bin DEPENDS ${BIOS1_BIN} bios2.bin)
add_custom_command(
    OUTPUT ${BIOS1_BIN}
    COMMAND ${CMAKE_ASM_NASM_COMPILER} -o ${BIOS1_BIN} boot1.asm
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS boot1.asm ${BIOS_HELPERS} ${BIOS2_BIN}
)

add_custom_target(bios2.bin DEPENDS ${BIOS2_BIN} diskmaker)
add_custom_command(
    OUTPUT ${BIOS2_BIN}
    COMMAND ${CMAKE_ASM_NASM_COMPILER} -o ${BIOS2_BIN} boot2.asm
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS boot2.asm ${BIOS_HELPERS}
)

# This abomination of a command is because CMake escaping is the dumbest thing I have ever seen
# It changes the BIOS2_SECTORS in constants.asm at compile time
add_custom_command(
    TARGET bios2.bin POST_BUILD
    COMMAND sed -i "\"s/BIOS2_SECTORS.*/BIOS2_SECTORS" "`$<TARGET_FILE:diskmaker>" "-s" "BIOS2`/\"" ${CMAKE_CURRENT_SOURCE_DIR}/constants.asm
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)