#! /usr/bin/env python3

import os, sys
from os import path

EFI = '@efi@'
ELF = '@elf@'
BIOS1 = '@bios1@'
BIOS2 = '@bios2@'

EFI_PATH = '/EFI/BOOT'
ELF_PATH = '/'

MFORMAT = '@mformat@'
MMD = '@mmd@'
MCOPY = '@mcopy@'
SGDISK = '@sgdisk@'
MAKE = '@make@'

SRC_ROOT = '@src_root@'
OUT = sys.argv[1]

limine_dir = SRC_ROOT + '/subprojects/limine'
os.system(f'{MAKE} -C {limine_dir}')
with open(OUT, 'w+b') as f:
    f.truncate(1024 * 1024 * 64)
os.system(f'{limine_dir}/limine bios-install {OUT}')
os.system(f'{MFORMAT} -i {OUT}@@1M')
os.system(f'{MMD} -i {OUT}@@1M ::/EFI ::/EFI/BOOT ::/boot :: /boot/limine')
