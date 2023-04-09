TARGET = pivotos.efi
SRCS = entry.c
ALSO = kernel.elf
CFLAGS += -I./kernel/include
include uefi/Makefile

.PHONY = run
run: $(TARGET) $(ALSO)
	uefi-run $(TARGET) -f $(ALSO)

$(ALSO):
	make -C kernel

clean: kernelclean
kernelclean:
	make -C uefi clean
	make -C kernel clean
	rm *.o *.efi *.elf
