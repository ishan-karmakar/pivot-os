add_languages('cpp')

c_args = args
cpp_args = args
linker_script = meson.current_source_dir() / 'linker.ld'
font = meson.global_source_root() / 'fonts/default.psf'

c_args += [
    '-static',
    '-mcmodel=kernel',
    '-fstack-protector'
]

cpp_args += [
    '-static',
    '-mcmodel=kernel',
    '-fstack-protector',
    '-fno-rtti',
    '-fno-exceptions'
]

link_args = cpp_args
link_args += [
    '-z', 'norelro',
    '-Wl,--build-id=none',
    '-T', linker_script
]

nasm_gen = generator(
    nasm,
    output: '@PLAINNAME@.o',
    arguments: [
        '-felf64',
        '@INPUT@',
        '-o', '@OUTPUT@'
    ]
)

objcopy = find_program('objcopy', required: true)
font_target = custom_target(
    'font',
    input: font,
    output: '@PLAINNAME@.o',
    command: [
        meson.global_source_root() / 'util/compile_font.py',
        objcopy,
        '@INPUT@',
        '@OUTPUT@'
    ]
)

uacpi = subproject('uacpi', required: true)
frigg = dependency(
    'frigg',
    default_options: ['frigg_no_install=true'],
    fallback: ['frigg', 'frigg_dep'],
    required: true
)

common_lib = static_library(
    'common',
    common_sources,
    include_directories: common_includes,
    c_args: c_args,
    pic: false
)

uacpi_includes = uacpi.get_variable('includes')
uacpi_lib = static_library(
    'uacpi',
    uacpi.get_variable('sources'),
    include_directories: uacpi_includes,
    c_args: c_args,
    pic: false
)

kernel = executable(
    'kernel.elf',
    'kernel.cpp',
    'init.cpp',
    'acpi/acpi.cpp',
    'cpu/gdt.cpp',
    'cpu/idt.cpp',
    'cpu/isr.cpp',
    'cpu/lapic.cpp',
    'cpu/tss.cpp',
    'drivers/framebuffer.cpp',
    'drivers/ioapic.cpp',
    'drivers/keyboard.cpp',
    'drivers/pit.cpp',
    'drivers/ps2.cpp',
    'drivers/rtc.cpp',
    'io/serial.cpp',
    'io/stdio.cpp',
    'mem/bitmap.cpp',
    'mem/heap.cpp',
    'mem/mapper.cpp',
    'mem/pmm.cpp',
    'mem/vmm.cpp',
    'util/string.cpp',
    nasm_gen.process('cpu/interrupts.asm'),
    nasm_gen.process('cpu/ap_trampoline.asm'),
    font_target,
    
    c_args: c_args,
    cpp_args: cpp_args,
    link_args: link_args,
    link_depends: linker_script,
    link_with: [common_lib, uacpi_lib],
    dependencies: frigg,
    implicit_include_directories: false,
    include_directories: ['../../include/kernel', common_includes, uacpi_includes],
    build_by_default: false
)