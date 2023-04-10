SHARED = pivotos.so
EFI = pivotos.efi
ISO = pivotos.iso
ELF = kernel.elf
ISO_DIR = image
SRCS = entry.c
OBJ = $(SRCS:.c=.o)
CFLAGS = -Ikernel/include -Ignu-efi/inc -fpic -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args
LDFLAGS = -shared -Bsymbolic -Lgnu-efi/x86_64/lib -Lgnu-efi/x86_64/gnuefi -Tgnu-efi/gnuefi/elf_x86_64_efi.lds gnu-efi/x86_64/gnuefi/crt0-efi-x86_64.o -lgnuefi -lefi
OBJFLAGS = -j .text -j .sdata -j .data -j .dynamic -j .dynsym  -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --target efi-app-x86_64 --subsystem=10
# .PHONY = run
# run: $(TARGET)
# 	uefi-run -d $(TARGET)
# -f $(ALSO)

all: run

run: $(ISO_DIR)
	qemu-system-x86_64 -bios /usr/share/ovmf/OVMF.fd -net none -hda fat:rw:$^

$(ISO_DIR): startup.nsh $(EFI) $(ELF)
	mkdir -p image
	cp $^ $@

$(EFI): gnuefi $(OBJ)
	ld $(LDFLAGS) $(OBJ) -o $(SHARED)
	objcopy $(OBJFLAGS) $(SHARED) $(EFI)

$(ELF):
	make -C kernel

gnuefi:
	make -C gnu-efi

%.o: %.c
	gcc $(CFLAGS) -c $< -o $@
