C_SRC := kernel/main.c # Change to find later
ASM_SRC := $(wildcard boot/*.asm)
C_OBJ := $(patsubst kernel/%.c, build/%.o, $(C_SRC))
ASM_OBJ := $(patsubst boot/%.asm, build/%.o, $(ASM_SRC))
CFLAGS := -ffreestanding \
        -O2 \
        -Wall \
        -Wextra \
        -mno-red-zone \
        -mno-sse \
        -mcmodel=large

all: build/os.iso

.PHONY = run
run: build/os.iso
	qemu-system-x86_64 -cdrom $^ -serial stdio

build/os.iso: build/kernel.bin grub.cfg
	mkdir -p build/isofiles/boot/grub
	cp grub.cfg build/isofiles/boot/grub
	cp build/kernel.bin build/isofiles/boot
	grub-mkrescue -o build/os.iso build/isofiles

build/kernel.bin: $(ASM_OBJ) $(C_OBJ)
	ld -n -o $@ -T linker.ld $^

build/%.o: boot/%.asm
	nasm -f elf64 $< -o $@

build/%.o: kernel/%.c
	x86_64-elf-gcc $(CFLAGS) -c $< -o $@
