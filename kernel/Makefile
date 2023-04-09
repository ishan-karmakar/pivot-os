KERNEL_C_SRC = $(wildcard *.c drivers/*.c libc/*.c cpu/*.c)
KERNEL_ASM_SRC = $(wildcard cpu/*.asm)
KERNEL_HEADERS = $(wildcard *.h drivers/*.h libc/*.h cpu/*.h)
KERNEL_OBJ = $(KERNEL_C_SRC:.c=.o) $(KERNEL_ASM_SRC:.asm=.o)
CFLAGS = -ffreestanding -fno-stack-protector -fno-stack-check -I include/ -Wall -Wextra -D __x86_64__
TARGET = ../kernel.elf

$(TARGET): $(KERNEL_OBJ)
	ld -nostdlib -z max-page-size=0x1000 -Ttext=0x01000000 $^ -o $@

%.o: %.c $(KERNEL_HEADERS)
	gcc $(CFLAGS) -c $< -o $@

%.o: %.asm
	nasm -felf64 $< -o $@

.PHONY = clean
clean:
	rm drivers/*.o
