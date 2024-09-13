BUILD_DIR := build
LIMINE_DIR := limine
TMP_DIR := /tmp/pivot-os
IN_ISO_FILES := $(addprefix limine/,BOOTAA64.EFI BOOTIA32.EFI BOOTLOONGARCH64.EFI BOOTRISCV64.EFI BOOTX64.EFI \
					limine-bios-cd.bin limine-bios-pxe.bin limine-bios.sys limine-uefi-cd.bin) limine.conf
OUT_ISO_FILES := $(addprefix $(TMP_DIR)/,$(notdir $(IN_ISO_FILES)))
CARGO_FLAGS :=
QEMU_FLAGS := -m 128M -smp 2 -serial stdio -no-reboot -no-shutdown
XORRISO_FLAGS := -as mkisofs -b limine-bios-cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table \
				 --efi-boot limine-uefi-cd.bin -efi-boot-part --efi-boot-image --protective-msdos-label
ISO := $(BUILD_DIR)/pivot-os.iso

all: iso

.PHONY: run
run: iso
	qemu-system-x86_64 $(QEMU_FLAGS) -drive file=$^,index=0,media=disk,format=raw

.PHONY: iso
iso: kernel $(OUT_ISO_FILES)
	mkdir -p build
	xorriso $(XORRISO_FLAGS) /tmp/pivot-os -o $(ISO)
	make -C $(LIMINE_DIR)
	$(LIMINE_DIR)/limine bios-install $(ISO)

.PHONY: kernel
kernel:
	mkdir -p $(TMP_DIR)
	cargo $(CARGO_FLAGS) build
	cp `cargo build --message-format=json 2> /dev/null | jq -r 'select((.target.kind == ["bin"]) and (.target.name == "pivot-os")) | .executable'` $(TMP_DIR)

$(OUT_ISO_FILES): $(IN_ISO_FILES)
	mkdir -p $(TMP_DIR)
	cp $^ $(TMP_DIR)
