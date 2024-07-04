project(bios ASM_NASM)
set(
    BIOS1_FILES
    boot1.asm
    screen16.asm
)

set(
    BIOS2_FILES
    boot2.asm
    screen32.asm
)

add_custom_target(bios1.bin DEPENDS ${BIOS1_BIN} bios2.bin)
add_custom_command(
    OUTPUT ${BIOS1_BIN}
    COMMAND ${CMAKE_ASM_NASM_COMPILER} -o ${BIOS1_BIN} boot1.asm
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${BIOS1_FILES} ${BIOS2_BIN}
)

add_custom_target(bios2.bin DEPENDS ${BIOS2_BIN} diskmaker)
add_custom_command(
    OUTPUT ${BIOS2_BIN}
    COMMAND ${CMAKE_ASM_NASM_COMPILER} -o ${BIOS2_BIN} boot2.asm
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${BIOS2_FILES}
)

# This abomination of a command is because CMake escaping is the dumbest thing I have ever seen
# It changes the BIOS2_SECTORS in constants.asm at compile time
add_custom_command(
    TARGET bios2.bin POST_BUILD
    COMMAND sed -i "\"s/BIOS2_SECTORS.*/BIOS2_SECTORS" "`$<TARGET_FILE:diskmaker>" "-s" "BIOS2`/\"" ${CMAKE_CURRENT_SOURCE_DIR}/constants.asm
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)