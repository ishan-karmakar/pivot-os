file(GLOB_RECURSE ASM_FILES CONFIGURE_DEPENDS "*.asm")
file(GLOB_RECURSE C_FILES CONFIGURE_DEPENDS "*.c")

set(PSF_OUT ${CMAKE_CURRENT_BINARY_DIR}/default.o)
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcmodel=large")

add_executable(
    kernel.elf
    EXCLUDE_FROM_ALL
    ${C_FILES}
    ${ASM_FILES}
    ${COMMON_FILES}
    ${PSF_OUT}
)

add_custom_command(
    OUTPUT ${PSF_OUT}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMAND ${CMAKE_OBJCOPY} -O elf64-x86-64 -B i386 -I binary fonts/default.psf ${PSF_OUT}
    VERBATIM
)

target_include_directories(kernel.elf PRIVATE ${CMAKE_SOURCE_DIR}/include/kernel)

set_target_properties(kernel.elf PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})
target_link_options(kernel.elf PRIVATE -fuse-ld=lld -no-pie -z max-page-size=0x1000 -T ${LINKER_SCRIPT})