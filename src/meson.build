add_languages('c', 'cpp')

nasm = find_program('nasm', required: true)
linker_script = meson.current_source_dir() / 'linker.ld'

nasm_gen = generator(
    nasm,
    output: '@PLAINNAME@.o',
    arguments: [
        '-felf64',
        '@INPUT@',
        '-o', '@OUTPUT@'
    ]
)

sources = [
    'kernel.elf',
    'kernel.cpp',
    'ap.cpp',
    'misc/cxxabi.cpp',
    'misc/mem.cpp',
    'misc/ubsan.cpp',
    'misc/string.cpp',
    'misc/strchr.cpp',
    'lib/logger.cpp',
    'lib/timer.cpp',
    'lib/uacpi.cpp',
    'lib/scheduler.cpp',
    'lib/interrupts.cpp',
    'lib/modules.cpp',
    'lib/syscalls.cpp',
    'lib/proc.cpp',
    'cpu/cpu.cpp',
    'cpu/gdt.cpp',
    'cpu/idt.cpp',
    'cpu/isr.cpp',
    'cpu/tss.cpp',
    'cpu/smp.cpp',
    'drivers/lapic.cpp',
    'drivers/term.cpp',
    'drivers/ioapic.cpp',
    'drivers/keyboard.cpp',
    'drivers/pit.cpp',
    'drivers/pic.cpp',
    'drivers/ps2.cpp',
    'drivers/rtc.cpp',
    'drivers/acpi.cpp',
    'drivers/madt.cpp',
    'drivers/pci.cpp',
    'drivers/initrd.cpp',
    'drivers/ustar.cpp',
    'drivers/fs/vfs.cpp',
    'drivers/fs/tmpfs.cpp',
    'drivers/fs/devtmpfs.cpp',
    'drivers/qemu.cpp',
    'io/serial.cpp',
    'io/stdio.cpp',
    'mem/heap.cpp',
    'mem/mapper.cpp',
    'mem/pmm.cpp',
    'mem/vmm.cpp',
    nasm_gen.process('cpu/intr.asm'),
    nasm_gen.process('lib/proc.asm')
]

options = []

# if get_option('buildtype').startswith('debug')
#     options += 'b_sanitize=undefined'
# endif

c_args = [
    '-ffreestanding',
    '-nostdlib',
    '-mno-red-zone',
    '-static',
    '-mno-mmx',
    '-mno-sse',
    '-mno-sse2',
    '-DUACPI_FORMATTED_LOGGING'
]

if (get_option('log_level') != '')
    c_args += '-DLOG_LEVEL=' + get_option('log_level').to_upper()
endif

cpp_args = [
    c_args,
    '-fno-exceptions',
]

link_args = cpp_args
link_args += [
    '-Wl,--build-id=none',
    '-Wl,-z,max-page-size=0x1000',
    '-T', linker_script
]

kernel = executable(
    sources,    
    c_args: c_args,
    cpp_args: cpp_args,
    link_args: link_args,
    link_depends: linker_script,
    dependencies: deps,
    implicit_include_directories: false,
    include_directories: '../include',
    build_by_default: false,
    override_options: options,
    pie: false
)
