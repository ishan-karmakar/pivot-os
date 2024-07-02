KERNEL_TARGET := $(BUILD_DIR)/kernel.elf
KERNEL_C_SRC := $(shell find src/kernel src/common -type f -name "*.c")
KERNEL_ASM_SRC := $(shell find src/kernel -type f -name "*.asm")
KERNEL_ASM_OBJ := $(patsubst src/kernel/%.asm, $(BUILD_DIR)/kernel/%.o, $(KERNEL_ASM_SRC))
KERNEL_FONT_OBJ := $(BUILD_DIR)/fonts/default.o
KERNEL_LINKER_FILE := src/kernel/linker.ld
KERNEL_FLAGS := -Iinclude/kernel -Iinclude/common -no-pie -ffreestanding -nostdlib \
	-Wall -Wextra -mno-red-zone -mno-sse -mcmodel=large -fuse-ld=lld \
	-fno-stack-protector -T$(KERNEL_LINKER_FILE) -z max-page-size=0x1000

$(KERNEL_TARGET): $(KERNEL_C_SRC) $(KERNEL_ASM_OBJ) $(KERNEL_FONT_OBJ)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(KERNEL_FLAGS) -v $^ -o $@

$(BUILD_DIR)/kernel/%.o: src/kernel/%.asm
	@mkdir -p $(@D)
	nasm -felf64 $< -o $@

$(BUILD_DIR)/fonts/%.o: fonts/%.psf
	@mkdir -p $(@D)
	objcopy -O elf64-x86-64 -B i386 -I binary $< $@
