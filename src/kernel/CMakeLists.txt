project(kernel C CXX ASM_NASM)
include(${CMAKE_SOURCE_DIR}/uACPI/uacpi.cmake)

file(GLOB_RECURSE ASM_FILES CONFIGURE_DEPENDS "*.asm")
file(GLOB_RECURSE CXX_FILES CONFIGURE_DEPENDS "*.cpp")

set(PSF_OUT ${CMAKE_CURRENT_BINARY_DIR}/default.o)
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static -mcmodel=kernel -fstack-protector")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -mcmodel=kernel -fno-rtti -fno-exceptions")
add_executable(
    kernel
    EXCLUDE_FROM_ALL
    ${CXX_FILES}
    ${ASM_FILES}
    ${COMMON_FILES}
    ${PSF_OUT}
)
target_compile_features(kernel PRIVATE cxx_std_17)
set_target_properties(kernel PROPERTIES OUTPUT_NAME kernel.elf)

add_custom_command(
    OUTPUT ${PSF_OUT}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMAND ${CMAKE_OBJCOPY} -O elf64-x86-64 -B i386 -I binary fonts/default.psf ${PSF_OUT}
    DEPENDS ${CMAKE_SOURCE_DIR}/fonts/default.psf
    VERBATIM
)

target_include_directories(kernel PRIVATE ${CMAKE_SOURCE_DIR}/include/kernel ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(kernel PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})
target_link_options(kernel PRIVATE -fuse-ld=lld -z norelro -T ${LINKER_SCRIPT})

target_sources(
    kernel
    PRIVATE
    ${UACPI_SOURCES}
)

target_include_directories(
    kernel
    PRIVATE
    ${UACPI_INCLUDES}
)
