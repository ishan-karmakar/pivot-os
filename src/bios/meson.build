nasm_command = [
    nasm,
    '@INPUT@',
    '-o', '@OUTPUT@',
    '-i', meson.current_source_dir()
]

bios2 = custom_target(
    'bios2',
    input: 'boot2.asm',
    output: '@BASENAME@.bin',
    command: nasm_command,
    build_by_default: false
)

bios1 = custom_target(
    'bios1',
    input: 'boot1.asm',
    output: '@BASENAME@.bin',
    command: nasm_command,
    depends: bios2,
    build_by_default: false
)