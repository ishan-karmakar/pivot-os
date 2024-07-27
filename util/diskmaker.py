#! /usr/bin/env python3

import os, sys
from os import path

ELF = '@elf@'

EFI_PATH = '/EFI/BOOT'
ELF_PATH = '/'

MFORMAT = '@mformat@'
MMD = '@mmd@'
MCOPY = '@mcopy@'
SGDISK = '@sgdisk@'
MAKE = '@make@'

SRC_ROOT = '@src_root@'
OUT = sys.argv[1]

limine_dir = SRC_ROOT + '/limine'
os.system(f'{MAKE} -C {limine_dir}')
with open(OUT, 'w+b') as f:
    f.truncate(1024 * 1024 * 64)

os.system(f'{SGDISK} {OUT} -n 1:2048 -t 1:ef00')
os.system(f'{limine_dir}/limine bios-install {OUT}')
os.system(f'{MFORMAT} -i {OUT}@@1M')
os.system(f'{MMD} -i {OUT}@@1M ::/EFI ::/EFI/BOOT ::/boot ::/boot/limine')
os.system(f'{MCOPY} -i {OUT}@@1M {ELF} ::/boot')
os.system(f'{MCOPY} -i {OUT}@@1M {SRC_ROOT}/limine.cfg {limine_dir}/limine-bios.sys ::/boot/limine')
os.system(f'{MCOPY} -i {OUT}@@1M {limine_dir}/BOOTX64.EFI ::/EFI/BOOT')
os.system(f'{MCOPY} -i {OUT}@@1M {limine_dir}/BOOTIA32.EFI ::/EFI/BOOT')
