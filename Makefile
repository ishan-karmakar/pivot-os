TARGET = pivotos.efi
SRCS = entry.c
ALSO = kernel.elf
KERNEL_SRC = $(wildcard kernel/*.c)
KERNEL_OBJ = $(KERNEL_SRC:.c=.o)
KERNEL_HEADERS = $(wildcard kernel/*.h)
include posix-uefi/uefi/Makefile

.PHONY = run
run: $(TARGET) $(ALSO)
	uefi-run $(TARGET) -f $(ALSO)

$(ALSO): $(KERNEL_OBJ)
	ld -nostdlib -z max-page-size=0x1000 -Ttext=0x01000000 $^ -o $@

kernel/%.o: kernel/%.c $(KERNEL_HEADERS)
	gcc -ffreestanding -fno-stack-protector -fno-stack-check -D __$(ARCH)__ -c $< -o $@

clean: kernelclean
kernelclean:
	rm -f $(KERNEL_OBJ)
