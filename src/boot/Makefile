BOOT_C_SRC := $(shell find src/boot -type f -name "*.c")
BOOT_C_OBJ := $(patsubst src/boot/%.c, build/boot/%.o, $(BOOT_C_SRC))
GNU_EFI_INC := /usr/include/efi
GNU_EFI_LIB := /usr/lib
BOOT_CFLAGS :=  -I$(GNU_EFI_INC) \
				-Iinclude/ \
				-fpic \
				-ffreestanding \
				-nostdlib \
				-fno-stack-protector \
				-fno-stack-check \
				-mno-red-zone \
				-fshort-wchar \
				-maccumulate-outgoing-args \
				-Wall \
				-Wextra
BOOT_LDFLAGS := -shared \
				-Bsymbolic \
				-L$(GNU_EFI_LIB) \
				-T$(GNU_EFI_LIB)/elf_x86_64_efi.lds \
				$(GNU_EFI_LIB)/crt0-efi-x86_64.o \

build/BOOTX64.efi: $(BOOT_C_OBJ)
	ld $(BOOT_LDFLAGS) $^ -o build/main.so -lgnuefi -lefi
	objcopy -g -R .hash -R .gnu.hash -R .eh_frame -R .comment --target efi-app-x86_64 --subsystem=10 build/main.so $@

build/boot/%.o: src/boot/%.c
	@mkdir -p $(@D)
	gcc $(BOOT_CFLAGS) -c $< -o $@