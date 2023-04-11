# SHARED = pivotos.so
# EFI = pivotos.efi
# ISO = pivotos.iso
# ELF = kernel.elf
# ISO_DIR = image
# SRCS = entry.c
# OBJ = $(SRCS:.c=.o)
# CFLAGS = -Ikernel/include -Ignu-efi/inc -fpic -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args
# LDFLAGS = -shared -Bsymbolic -Lgnu-efi/x86_64/lib -Lgnu-efi/x86_64/gnuefi -Tgnu-efi/gnuefi/elf_x86_64_efi.lds gnu-efi/x86_64/gnuefi/crt0-efi-x86_64.o -lgnuefi -lefi
# OBJFLAGS = -j .text -j .sdata -j .data -j .dynamic -j .dynsym  -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --target efi-app-x86_64 --subsystem=10
# # .PHONY = run
# # run: $(TARGET)
# # 	uefi-run -d $(TARGET)
# # -f $(ALSO)

# all: run

# run: $(ISO_DIR)
# 	qemu-system-x86_64 -bios /usr/share/ovmf/OVMF.fd -net none -hda fat:rw:$^

# $(ISO_DIR): startup.nsh $(EFI) $(ELF)
# 	mkdir -p image
# 	cp $^ $@

# $(EFI): gnuefi $(OBJ)
# 	ld $(LDFLAGS) $(OBJ) -o $(SHARED)
# 	objcopy $(OBJFLAGS) $(SHARED) $(EFI)

# $(ELF):
# 	make -C kernel

# gnuefi:
# 	make -C gnu-efi

# %.o: %.c
# 	gcc $(CFLAGS) -c $< -o $@

ARCH            = $(shell uname -m | sed s,i[3456789]86,ia32,)
LIB_PATH        = /usr/lib64
EFI_INCLUDE     = /usr/local/include/efi
EFI_INCLUDES    = -I$(EFI_INCLUDE) -I$(EFI_INCLUDE)/$(ARCH) -I$(EFI_INCLUDE)/protocol

EFI_PATH        = /usr/local/lib
EFI_CRT_OBJS    = $(EFI_PATH)/crt0-efi-$(ARCH).o
EFI_LDS         = $(EFI_PATH)/elf_$(ARCH)_efi.lds

CFLAGS          = -ffreestanding -fno-stack-protector -fpic -fshort-wchar -mno-red-zone $(EFI_INCLUDES)
ifeq ($(ARCH),x86_64)
	CFLAGS  += -DEFI_FUNCTION_WRAPPER
endif

LDFLAGS         = -nostdlib -znocombreloc -T $(EFI_LDS) -shared -Bsymbolic -L$(EFI_PATH) -L$(LIB_PATH) \
                  $(EFI_CRT_OBJS) -lefi -lgnuefi

TARGET  = test.efi
OBJS    = entry.o
SOURCES = entry.c

all: $(TARGET)

test.so: $(OBJS)
	ld -o $@ $(LDFLAGS) $^ $(EFI_LIBS)

%.efi: %.so
	objcopy -j .text -j .sdata -j .data \
			-j .dynamic -j .dynsym  -j .rel \
			-j .rela -j .reloc -j .eh_frame \
			--target=efi-app-$(ARCH) $^ $@
