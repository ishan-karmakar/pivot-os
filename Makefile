BUILD_DIR := build
LIMINE_DIR := limine
LIMINE_ISO_FILES := BOOTAA64.EFI BOOTIA32.EFI BOOTLOONGARCH64.EFI BOOTRISCV64.EFI BOOTX64.EFI \
					limine-bios-cd.bin limine-bios-pxe.bin limine-bios.sys limine-uefi-cd.bin
ISO_FILES := $(addprefix $(LIMINE_DIR)/,$(LIMINE_ISO_FILES)) limine.conf
CARGO_FLAGS :=
QEMU_FLAGS := -m 128M -smp 2 -serial stdio -no-reboot -no-shutdown
XORRISO_FLAGS := -as mkisofs -b limine-bios-cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table \
				 --efi-boot limine-uefi-cd.bin -efi-boot-part --efi-boot-image --protective-msdos-label

all: $(BUILD_DIR)/pivot-os.iso

.PHONY: run
run: $(BUILD_DIR)/pivot-os.iso
	qemu-system-x86_64 $(QEMU_FLAGS) -drive file=$^,index=0,media=disk,format=raw

.PHONY: $(BUILD_DIR)/pivot-os.iso
$(BUILD_DIR)/pivot-os.iso:
	rm -rf /tmp/pivot-os
	mkdir -p /tmp/pivot-os build
	cp -r $(ISO_FILES) /tmp/pivot-os/
	cargo $(CARGO_FLAGS) build
	cp `cargo build --message-format=json 2> /dev/null | jq -r 'select((.target.kind == ["bin"]) and (.target.name == "pivot-os")) | .executable'` /tmp/pivot-os
	xorriso $(XORRISO_FLAGS) /tmp/pivot-os -o $@
	make -C $(LIMINE_DIR)
	$(LIMINE_DIR)/limine bios-install $@