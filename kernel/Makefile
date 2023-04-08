ARCH = x86_64
TARGET = ../kernel.elf
SOURCES = $(wildcard *.c)
OBJ = $(SOURCES:.c=.o)

$(TARGET): $(OBJ)
	

%.o: %.c
	gcc -ffreestanding -fno-stack-protector -fno-stack-check -D __$(ARCH)__ -c $< -o $@

.PHONY = clean
clean:
	rm -f $(OBJ) $(TARGET)
