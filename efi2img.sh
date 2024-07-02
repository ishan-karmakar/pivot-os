#! /bin/bash
set +x
set -e

IMG_SIZE=4M

BOOTLOADER_EFI=$1
KERNEL_ELF=$2
IMG=$3
# 1:(12)s:(12345)s:...
REGEX="^[0-9]:([0-9]+)s:([0-9]+)s"
# FIXME: Need to figure out how this all works and fix it
# Maybe use a python script or something
rm -f $IMG /tmp/part.img

truncate -s $IMG_SIZE $IMG
output=$(parted $IMG -m -s -a min mklabel gpt mkpart TEST FAT32 0% 100% unit s p list | sed -n 3p)
[[ $output =~ $REGEX ]]
start_sec=${BASH_REMATCH[1]}
end_sec=${BASH_REMATCH[2]}
bytes=$(( (end_sec - start_sec) * 512 ))

truncate -s $bytes /tmp/part.img
mkfs.fat /tmp/part.img
mmd -i /tmp/part.img ::/EFI
mmd -i /tmp/part.img ::/EFI/BOOT
mcopy -i /tmp/part.img $BOOTLOADER_EFI ::/EFI/BOOT
mcopy -i /tmp/part.img $KERNEL_ELF ::/

dd if=/tmp/part.img of=$IMG seek=$start_sec conv=notrunc