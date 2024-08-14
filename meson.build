project(
    'pivot-os',
    default_options: [
        'c_std=gnu2x',
        'cpp_std=gnu++20',
        'warning_level=2',
        'cpp_rtti=false',
    ]
)

env = environment()
limine = subproject('limine', required: true)
deps = [
    dependency('uacpi', required: true),
    dependency('frigg', default_options: ['frigg_no_install=true'], required: true),
    dependency('magic_enum', default_options: ['hash=true', 'test=false'], required: true),
    dependency('buddy', required: true),
    dependency('fmt', required: true),
    limine.get_variable('limine_dep'),
]

env.set('objcopy', find_program('objcopy', required: true).full_path())
font_target = custom_target(
    'font',
    input: 'fonts/default.psf',
    output: '@PLAINNAME@.o',
    command: [
        'util/compile_font.py',
        '@INPUT@',
        '@OUTPUT@'
    ],
    env: env
)

subdir('src')

env.set('mmd', find_program('mmd', required: true).full_path())
env.set('mcopy', find_program('mcopy', required: true).full_path())
env.set('mformat', find_program('mformat', required: true).full_path())
env.set('sgdisk', find_program('sgdisk', required: true).full_path())
env.set('elf', kernel.full_path())
env.set('src_root', meson.global_source_root())

limine_exec = limine.get_variable('limine_exec')
env.set('limine', limine_exec.full_path())
os = custom_target(
    'os',
    output: 'os.img',
    command: ['util/diskmaker.py', '@OUTPUT@'],
    depends: [limine_exec, kernel],
    env: env,
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
