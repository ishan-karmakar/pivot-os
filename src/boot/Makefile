BOOT_TARGET := build/BOOTX64.efi
BOOT_C_SRC := $(shell find src/boot -type f -name "*.c")
BOOT_CFLAGS :=  -I$(GNU_EFI_INC) \
				-Iinclude/ \
				-fpic \
				-ffreestanding \
				-nostdlib \
				-fno-stack-protector \
				-mno-red-zone \
				-fshort-wchar \
				-Wall \
				-Wextra
BOOT_LDFLAGS := -shared \
				-Bsymbolic \
				-L$(GNU_EFI_LIB) \
				-T$(GNU_EFI_LIB)/elf_x86_64_efi.lds \
				$(GNU_EFI_LIB)/crt0-efi-x86_64.o
BOOT_FLAGS := -Iinclude/ -ffreestanding -nostdlib -fno-stack-protector -mno-red-zone \
	-fshort-wchar -target x86_64-unknown-windows -fuse-ld=lld \
	-Wl,-entry:efi_main,-subsystem:efi_application -Wall -Wextra

$(BOOT_TARGET): $(BOOT_C_SRC) $(HEADER_FILES)
	@mkdir -p build
	$(CC) $(BOOT_FLAGS) $(BOOT_C_SRC) -o $@
