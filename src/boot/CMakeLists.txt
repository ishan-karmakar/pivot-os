project(bootloader C)

file(GLOB_RECURSE C_FILES CONFIGURE_DEPENDS "*.c")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -target x86_64-unknown-windows -fshort-wchar")

add_executable(
    BOOTX64.efi
    EXCLUDE_FROM_ALL
    ${C_FILES}
    ${COMMON_FILES}
)

target_include_directories(BOOTX64.efi PRIVATE ${CMAKE_SOURCE_DIR}/include/boot)
target_link_options(BOOTX64.efi PRIVATE -fuse-ld=lld -Wl,-entry:efi_main,-subsystem:efi_application)