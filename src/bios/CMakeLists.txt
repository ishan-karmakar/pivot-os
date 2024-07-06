project(bios C ASM_NASM)
set(
    BIOS_HELPERS
    common.asm
    util16.asm
)

add_custom_target(bios1 DEPENDS bios1.bin bios2)
add_custom_command(
    OUTPUT bios1.bin
    COMMAND ${CMAKE_ASM_NASM_COMPILER} -o ${CMAKE_CURRENT_BINARY_DIR}/bios1.bin boot1.asm
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS boot1.asm ${BIOS_HELPERS} bios2.bin
)

add_custom_target(bios2 DEPENDS bios2.bin diskmaker)
add_custom_command(
    OUTPUT bios2.bin
    COMMAND ${CMAKE_ASM_NASM_COMPILER} -o ${CMAKE_CURRENT_BINARY_DIR}/bios2.bin boot2.asm
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS boot2.asm ${BIOS_HELPERS}
)

# This abomination of a command is because CMake escaping is the dumbest thing I have ever seen
# It changes the BIOS2_SECTORS in constants.asm at compile time
add_custom_command(
    TARGET bios2 POST_BUILD
    COMMAND sed -i "\"s/BIOS2_SECTORS.*/BIOS2_SECTORS" "`$<TARGET_FILE:diskmaker>" "-s" "BIOS2`/\"" ${CMAKE_CURRENT_SOURCE_DIR}/constants.asm
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)