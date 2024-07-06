project(uefi C)

file(GLOB_RECURSE C_FILES CONFIGURE_DEPENDS "*.c")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -target x86_64-unknown-windows -fshort-wchar")

add_executable(
    uefi
    EXCLUDE_FROM_ALL
    ${C_FILES}
    ${COMMON_FILES}
)

set_target_properties(uefi PROPERTIES OUTPUT_NAME BOOTX64.efi)

target_include_directories(uefi PRIVATE ${CMAKE_SOURCE_DIR}/include/boot)
target_link_options(uefi PRIVATE -fuse-ld=lld -Wl,-entry:efi_main,-subsystem:efi_application)