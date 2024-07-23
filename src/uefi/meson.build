project('uefi', 'c', version: '0.1.0', default_options: ['warning_level=3'])
if meson.get_compiler('c').get_id() != 'clang'
    warning('Consider using Clang. This project has only been tested on Clang and is known not to work with GCC')
endif
add_global_arguments(['-ffreestanding', '-fno-stack-protector', '-nostdlib', '-mno-red-zone'], language: 'c')
add_project_arguments(['-target', 'x86_64-unknown-windows', '-fshort-wchar'], language: 'c')
executable(
    'BOOTX64.efi',
    'uefi.c',
    'mem.c',
    'loader.c',
    'graphics.c',
    'con.c',
    'acpi.c',
    link_args: ['-ffreestanding', '-fno-stack-protector', '-nostdlib', '-mno-red-zone', '-target', 'x86_64-unknown-windows', '-fshort-wchar', '-fuse-ld=lld', '-Wl,-entry:efi_main,-subsystem:efi_application'],
    include_directories: [
        include_directories('../../include/uefi'),
        include_directories('../../include/common')
    ]
)