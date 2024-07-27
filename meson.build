project(
    'pivot-os',
    default_options: [
        'b_asneeded=false',
        'c_std=gnu2x',
        'cpp_std=gnu++20',
        'warning_level=2'
    ]
)

args = [
    '-ffreestanding',
    '-nostdlib',
    '-mno-red-zone'
]

data = configuration_data()
nasm = find_program('nasm', required: true)
python3 = find_program('python3', required: true)
qemu = find_program('qemu-system-x86_64', required: false)
data.set('mmd', find_program('mmd', required: true).full_path())
data.set('mcopy', find_program('mcopy', required: true).full_path())
data.set('mformat', find_program('mformat', required: true).full_path())
data.set('sgdisk', find_program('sgdisk', required: true).full_path())
data.set('make', find_program('make', required: true).full_path())

debug = qemu.found() and get_option('buildtype') in ['debug', 'debugoptimized', 'plain']
if debug
    args += ['-DDEBUG']
endif

frigg = dependency(
    'frigg',
    default_options: ['frigg_no_install=true'],
    fallback: ['frigg', 'frigg_dep'],
    required: true
)

subdir('src/common')
# subdir('src/uefi')
# subdir('src/bios')
subdir('src/kernel')

# data.set('bios1', bios1.full_path())
# data.set('bios2', bios2.full_path())
# data.set('efi', uefi.full_path())
data.set('elf', kernel.full_path())
data.set('src_root', meson.global_source_root())

diskmaker = configure_file(input: 'util/diskmaker.py', output: 'diskmaker.py', configuration: data)

os = custom_target(
    'os',
    output: 'os.img',
    command: [python3, diskmaker, '@OUTPUT@'],
    depends: kernel,
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