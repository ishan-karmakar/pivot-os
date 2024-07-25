c_args = args
c_args += [
    '-fno-stack-protector',
    '-target', 'x86_64-unknown-windows',
    '-fshort-wchar'
]

link_args = c_args
link_args += [
    '-Wl,-entry:efi_main,-subsystem:efi_application'
]

uefi = executable(
    'BOOTX64.efi',
    'uefi.c',
    'mem.c',
    'loader.c',
    'graphics.c',
    'con.c',
    'acpi.c',
    common_files,

    c_args: c_args,
    link_args: link_args,
    implicit_include_directories: false,
    include_directories: [
        '../../include/uefi',
        common_inc
    ],
    build_by_default: false
)