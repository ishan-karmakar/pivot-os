project(
    'pivot-os',
    'c', 'cpp',
    default_options: [
        'b_asneeded=false',
        'b_lundef=false',
        'c_std=gnu17',
        'cpp_std=gnu++17'
    ]
)

args = [
    '-ffreestanding',
    '-nostdlib',
    '-mno-red-zone'
]

data = configuration_data()
uacpi = subproject('uacpi', required: true)
nasm = find_program('nasm', required: true)
python3 = find_program('python3', required: true)
qemu = find_program('qemu-system-x86_64', required: false)
debug = qemu.found() and get_option('buildtype') in ['debug', 'debugoptimized', 'plain']
data.set('mkfs', find_program('mkfs.fat', required: true).full_path())
data.set('mmd', find_program('mmd', required: true).full_path())
data.set('mcopy', find_program('mcopy', required: true).full_path())
data.set('sgdisk', find_program('sgdisk', required: true).full_path())

if debug
    args += ['-DDEBUG']
endif

subdir('src/common')
subdir('src/uefi')
subdir('src/bios')
subdir('src/kernel')

data.set('bios1', bios1.full_path())
data.set('bios2', bios2.full_path())
data.set('efi', uefi.full_path())
data.set('elf', kernel.full_path())

diskmaker = configure_file(
    input: 'util/diskmaker.py',
    output: 'diskmaker.py',
    configuration: data
)

os = custom_target(
    'os',
    output: 'os.img',
    command: [python3, diskmaker, '@OUTPUT@'],
    depends: [bios1, bios2, uefi, kernel],
    build_by_default: true
)

if debug
    qemu_flags = [
        '-m', '128M',
        '-smp', '2',
        '-serial', 'stdio',
        '-no-reboot',
        '-no-shutdown'
    ]
    if get_option('qemu_uefi')
        qemu_flags += ['-bios', 'OVMF.fd']
    endif

    run_target(
        'run',
        command: [qemu, qemu_flags, '-drive', 'file=@0@,index=0,media=disk,format=raw'.format(os.full_path())],
        depends: os
    )
endif