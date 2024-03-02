all: KERNEL_LDFLAGS += -S -s
all: BOOT_LDFLAGS += -S -s
all: build/os.img
include src/boot/Makefile
include src/kernel/Makefile

.PHONY = run debug-run clean

run: KERNEL_LDFLAGS += -S -s
run: BOOT_LDFLAGS += -S -s
run: build/os.img
	qemu-system-x86_64 -m 128M -serial stdio -bios OVMF.fd -no-reboot -no-shutdown -drive file=$<,index=0,media=disk,format=raw

debug: KERNEL_CFLAGS += -g
debug: BOOT_CFLAGS += -g
debug: build/os.img
	qemu-system-x86_64 -S -s -m 128M -serial stdio -bios OVMF.fd -drive file=$<,index=0,media=disk,format=raw

build/os.img: build/BOOTX64.efi build/kernel.elf
	./efi2img.sh $^ $@

clean:
	rm -rf build/
