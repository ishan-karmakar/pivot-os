#! /usr/bin/env python3

import os, sys
from os import path

EFI = '@efi@'
ELF = '@elf@'
BIOS1 = '@bios1@'
BIOS2 = '@bios2@'

EFI_PATH = '/EFI/BOOT'
ELF_PATH = '/'

FAT_MKFS = '@mkfs@'
MMD = '@mmd@'
MCOPY = '@mcopy@'
SGDISK = '@sgdisk@'

NUM_SUBDIRS = 1

def get_size(p):
    return int((path.getsize(p) + 511) / 512)

def create_subdirectories(d, p):
    e = d
    ancestors = []
    while path.dirname(d) != d:
        ancestors.append(d)
        d = path.dirname(d)
    ancestors.reverse()
    for a in ancestors:
        os.system(f"{MMD} -i tmp.img ::{a}")
    os.system(f"{MCOPY} -i tmp.img {p} ::{e}")

def copy_file(ofs, ifs, seek, skip, blocks, bs):
    print(f"Copying from {ifs}, skipping {skip} * {bs}, seeking {seek} * {bs}, size: {blocks} * {bs}")
    ifs = open(ifs, 'rb')
    ifs.seek(skip * bs)
    ofs.seek(seek * bs)
    for _ in range(blocks):
        buf = ifs.read(bs)
        ofs.write(buf)
    ifs.close()

try:
    os.remove('tmp.img')
except OSError:
    pass

out = sys.argv[1]
f = open(out, 'w+b')
efi_size = get_size(EFI)
elf_size = get_size(ELF)
b2_size = get_size(BIOS2)
fat_size = 16 + 4 + b2_size + NUM_SUBDIRS + efi_size + elf_size
fat_size = max(69, fat_size)
os_size = 34 + 33 + fat_size

os.system(f"{FAT_MKFS} -f 1 -s 1 -a -r 16 -F12 -R {b2_size + 1} -C tmp.img {int((fat_size + 1) / 2)}")
create_subdirectories(EFI_PATH, EFI)
create_subdirectories(ELF_PATH, ELF)
f.truncate(os_size * 512)
os.system(f"{SGDISK} -N 1 -t 1:ef00 -h 1 {out} > /dev/null")
print("Created GPT label")
copy_file(f, BIOS1, 0, 0, 446, 1)
copy_file(f, BIOS1, 510, 510, 2, 1)
copy_file(f, 'tmp.img', 34, 0, fat_size, 512)
copy_file(f, BIOS2, 35, 0, b2_size, 512)
f.close()