set +x

# In sectors (512 bytes)
IMG_SIZE=93750
PADDING=64
END_SECTOR=93716 # Not sure how to figure out on the fly, I just created a test img and saw the end sector
PART_SIZE=$((END_SECTOR - PADDING))

BOOTLOADER_EFI=$1
KERNEL_EFI=$2
IMG=$3

rm -f $IMG /tmp/part.img
dd if=/dev/zero of=$IMG bs=512 count=$IMG_SIZE
parted $IMG -s -a minimal mklabel gpt
parted $IMG -s -a minimal mkpart EFI FAT32 ${PADDING}s ${END_SECTOR}s

dd if=/dev/zero of=/tmp/part.img bs=512 count=$PART_SIZE
mformat -i /tmp/part.img -h 32 -t 32 -n 64 -c 1
mmd -i /tmp/part.img ::/EFI
mmd -i /tmp/part.img ::/EFI/BOOT
mcopy -i /tmp/part.img $BOOTLOADER_EFI ::/EFI/BOOT
mcopy -i /tmp/part.img $KERNEL_EFI ::/

dd if=/tmp/part.img of=$IMG bs=512 count=$PART_SIZE seek=$PADDING conv=notrunc
