C_SRC := $(shell find src/ -type f -name "*.c") # Change to find later
ASM_SRC := $(shell find src/ -type f -name "*.asm")
C_OBJ := $(patsubst src/%.c, build/%.o, $(C_SRC))
ASM_OBJ := $(patsubst src/%.asm, build/%.o, $(ASM_SRC))
FONT_OBJ := build/fonts/default.o
CFLAGS := -ffreestanding \
		-I src/include \
        -O3 \
        -Wall \
        -Wextra \
        -mno-red-zone \
        -mno-sse \
        -mcmodel=large

all: build/os.iso
.PHONY = run clean

run: build/os.iso
	qemu-system-x86_64 -smp 2 -cdrom $^ -serial stdio

build/os.iso: build/kernel.bin grub.cfg
	mkdir -p build/isofiles/boot/grub
	cp grub.cfg build/isofiles/boot/grub
	cp build/kernel.bin build/isofiles/boot
	grub-mkrescue -o build/os.iso build/isofiles

build/kernel.bin: $(ASM_OBJ) $(C_OBJ) $(FONT_OBJ)
	ld -n -o $@ -T linker.ld $^

build/%.o: src/%.asm
	mkdir -p $(@D)
	nasm -f elf64 $< -o $@

build/%.o: src/%.c
	mkdir -p $(@D)
	x86_64-elf-gcc $(CFLAGS) -c $< -o $@

build/%.o: %.psf
	mkdir -p $(@D)
	objcopy -O elf64-x86-64 -B i386 -I binary $< $@

clean:
	rm -rf build/
