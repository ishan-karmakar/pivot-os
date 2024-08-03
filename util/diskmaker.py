#! /usr/bin/env python3

import os, sys
from os import environ

ELF = environ['elf']

EFI_PATH = '/EFI/BOOT'
ELF_PATH = '/'

SGDISK = environ['sgdisk']
MFORMAT = environ['mformat']
MMD = environ['mmd']
MCOPY = environ['mcopy']

OUT = sys.argv[1]

SRC_ROOT = environ['src_root']
LIMINE_EXEC = environ['limine']
limine_dir = SRC_ROOT + '/subprojects/limine'
with open(OUT, 'w+b') as f:
    f.truncate(1024 * 1024 * 64)

os.system(f'{SGDISK} {OUT} -n 1:2048 -t 1:ef00')
os.system(f'{LIMINE_EXEC} bios-install {OUT}')
os.system(f'{MFORMAT} -i {OUT}@@1M')
os.system(f'{MMD} -i {OUT}@@1M ::/EFI ::/EFI/BOOT ::/boot ::/boot/limine')
os.system(f'{MCOPY} -i {OUT}@@1M {ELF} ::/boot')
os.system(f'{MCOPY} -i {OUT}@@1M {SRC_ROOT}/limine.cfg {limine_dir}/limine-bios.sys ::/boot/limine')
os.system(f'{MCOPY} -i {OUT}@@1M {limine_dir}/BOOTX64.EFI ::/EFI/BOOT')
os.system(f'{MCOPY} -i {OUT}@@1M {limine_dir}/BOOTIA32.EFI ::/EFI/BOOT')
