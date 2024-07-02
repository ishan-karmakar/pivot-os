CC := clang
QEMU_FLAGS := -m 128M -smp 2 -enable-kvm -serial stdio -bios OVMF.fd -no-reboot -no-shutdown
HEADER_FILES := $(shell find include -type f -name "*.h")
IMG_SIZE := 4M
REGEX := ^[0-9]:([0-9]+)s:([0-9]+)s
BUILD_DIR := build/base

all: $(BUILD_DIR)/os.img
include src/boot/Makefile
include src/kernel/Makefile

.PHONY = run debug clean

run: BUILD_DIR := build/run
run: base-run

debug: QEMU_FLAGS += -s
debug: BUILD_DIR := build/debug
debug: base-run

base-run: BOOT_FLAGS += -DQEMU
base-run: $(BUILD_DIR)/os.img
	qemu-system-x86_64 $(QEMU_FLAGS) -drive file=$<,index=0,media=disk,format=raw

$(BUILD_DIR)/os.img: $(BOOT_TARGET) $(KERNEL_TARGET)
	@echo $(BUILD_DIR)
	@echo If switching between "make" and "make run/debug", clean and build to recompile with correct QEMU flags
	./efi2img.sh $^ $@

clean:
	rm -rf build/
