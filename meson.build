project(
    'pivot-os',
    default_options: [
        'c_std=gnu2x',
        'cpp_std=gnu++20',
        'warning_level=2',
        'cpp_rtti=false',
    ]
)

limine = subproject('limine', required: true)
deps = [
    dependency('uacpi', required: true),
    dependency('frigg', default_options: ['frigg_no_install=true'], required: true),
    dependency('magic_enum', default_options: ['hash=true', 'test=false'], required: true),
    dependency('buddy', required: true),
    dependency('limine-terminal', required: true),
    limine.get_variable('limine_dep'),
]

subdir('src')

iso_files = files(
    'limine.conf',
    'font.bin'
)

iso_files += limine.get_variable('limine_binaries')

os = custom_target(
    'os',
    output: 'os.iso',
    command: [
        'rm', '-rf', '@PRIVATE_DIR@', '&&',
        'mkdir', '@PRIVATE_DIR@', '&&',
        'cp', iso_files, kernel, '@PRIVATE_DIR@', '&&',
        'xorriso', '-as', 'mkisofs', '-b', 'limine-bios-cd.bin', '-no-emul-boot',
            '-boot-load-size', '4', '-boot-info-table', '--efi-boot', 'limine-uefi-cd.bin',
            '-efi-boot-part', '--efi-boot-image', '--protective-msdos-label', '@PRIVATE_DIR@',
            '-o', '@OUTPUT@', '&&',
        limine.get_variable('limine_exec'), 'bios-install', '@OUTPUT@'
    ],
    build_by_default: true
)

qemu = find_program('qemu-system-x86_64', required: false)
if qemu.found()
    qemu_flags = [
        '-m', '128M',
        '-smp', '2',
        '-serial', 'stdio',
        '-no-reboot',
        '-no-shutdown',
    ]

    if get_option('qemu_uefi')
        qemu_flags += ['-bios', 'OVMF.fd']
    endif

    if get_option('qemu_int')
        qemu_flags += ['-d', 'int']
    else
        qemu_flags += ['-enable-kvm', '-cpu', 'host']
    endif

    if get_option('qemu_debug')
        qemu_flags += ['-s', '-S']
    endif

    run_target(
        'run',
        command: [qemu, qemu_flags, '-drive', 'file=@0@,index=0,media=disk,format=raw'.format(os.full_path())],
        depends: os
    )
endif
