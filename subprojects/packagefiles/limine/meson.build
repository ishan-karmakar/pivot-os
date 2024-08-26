project('limine', 'c')

limine_includes = include_directories('.')

limine_exec = executable(
    'limine',
    'limine.c',
    include_directories: limine_includes
)

limine_binaries = files(
    'BOOTX64.EFI',
    'BOOTIA32.EFI',
    'BOOTRISCV64.EFI',
    'limine-bios.sys',
    'limine-bios-cd.bin',
    'limine-uefi-cd.bin'
)

limine_dep = declare_dependency(
	include_directories: limine_includes
)
