#! /usr/bin/env python3

import os, sys, re
from hashlib import blake2b
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
PRIVATE_DIR = sys.argv[2]
LIMINE_EXEC = environ['limine']
limine_dir = SRC_ROOT + '/subprojects/limine'

conf_out = open(f"{PRIVATE_DIR}/limine.conf", 'w+')
conf_in = open(f"{SRC_ROOT}/limine.conf", 'r')
with open(ELF, 'rb') as f:
    elf_hash = blake2b(f.read()).hexdigest()

conf_out.write(re.sub(r'^kernel_path:(.*kernel\.elf)$', f'kernel_path:\\1#{elf_hash}', conf_in.read(), flags=re.MULTILINE))
conf_out.close()
conf_in.close()

with open(OUT, 'w+b') as f:
    f.truncate(1 * (1024 * 1024 * 16)) # 1MB

os.system(f'{SGDISK} {OUT} -n 1:2048 -t 1:ef00')
os.system(f'{LIMINE_EXEC} bios-install {OUT}')
os.system(f'{MFORMAT} -i {OUT}@@1M')
os.system(f'{MMD} -i {OUT}@@1M ::/EFI ::/EFI/BOOT ::/boot ::/boot/limine')
os.system(f'{MCOPY} -i {OUT}@@1M {ELF} ::/boot')
os.system(f'{MCOPY} -i {OUT}@@1M {PRIVATE_DIR}/limine.conf {limine_dir}/limine-bios.sys ::/boot/limine')
os.system(f'{MCOPY} -i {OUT}@@1M {limine_dir}/BOOTX64.EFI ::/EFI/BOOT')
os.system(f'{MCOPY} -i {OUT}@@1M {limine_dir}/BOOTIA32.EFI ::/EFI/BOOT')
