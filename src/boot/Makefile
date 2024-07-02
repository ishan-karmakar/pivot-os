BOOT_TARGET := $(BUILD_DIR)/BOOTX64.efi
BOOT_C_SRC := $(shell find src/boot src/common -type f -name "*.c")
BOOT_FLAGS := -Iinclude/boot -Iinclude/common -ffreestanding -nostdlib -fno-stack-protector -mno-red-zone \
	-fshort-wchar -target x86_64-unknown-windows -fuse-ld=lld \
	-Wl,-entry:efi_main,-subsystem:efi_application -Wall -Wextra

$(BOOT_TARGET): $(BOOT_C_SRC) $(HEADER_FILES)
	mkdir -p $(BUILD_DIR)
	$(CC) $(BOOT_FLAGS) $(BOOT_C_SRC) -o $@
