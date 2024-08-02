add_languages('c', 'cpp')

nasm = find_program('nasm', required: true)
linker_script = meson.current_source_dir() / 'linker.ld'

c_args = [
    '-ffreestanding',
    '-nostdlib',
    '-mno-red-zone',
    '-static',
    '-fno-stack-protector',
    '-mno-sse'
]

cpp_args = [
    c_args,
    '-fno-rtti',
    '-fno-exceptions',
]

link_args = cpp_args
link_args += [
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

kernel = executable(
    'kernel.elf',
    'kernel.cpp',
    'misc/cxxabi.cpp',
    'misc/mem.cpp',
    'cpu/gdt.cpp',
    'cpu/idt.cpp',
    'cpu/isr.cpp',
    'cpu/tss.cpp',
    'cpu/smp.cpp',
    'drivers/lapic.cpp',
    'drivers/framebuffer.cpp',
    'drivers/ioapic.cpp',
    'drivers/keyboard.cpp',
    'drivers/pit.cpp',
    'drivers/ps2.cpp',
    'drivers/rtc.cpp',
    'drivers/acpi.cpp',
    'io/serial.cpp',
    'io/stdio.cpp',
    'mem/bitmap.cpp',
    'mem/heap.cpp',
    'mem/mapper.cpp',
    'mem/pmm.cpp',
    'mem/vmm.cpp',
    'util/logger.cpp',
    nasm_gen.process('cpu/interrupts.asm'),
    nasm_gen.process('cpu/ap_trampoline.asm'),
    font_target,
    
    c_args: c_args,
    cpp_args: cpp_args,
    link_args: link_args,
    link_depends: linker_script,
    dependencies: [uacpi, frigg, limine.get_variable('limine_dep')],
    implicit_include_directories: false,
    include_directories: '../include',
    build_by_default: false,
    pie: false
)