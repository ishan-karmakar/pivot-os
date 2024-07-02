import sys
SEC_SIZE = 512
EFI, ELF, OUT = sys.argv[1:]

def write_chs(file, lba):
    c = int(lba / (63 * 16))
    h = int(lba / 63) % 16
    s = lba % 63 + 1
    h &= 0xFF
    s &=0x3F
    c &= 0x3FF
    s = ((c & 0b11) << 6) | s
    c &= 0xFF
    file.write(bytes([h, s, c]))

def make_protective_mbr(file, num_sectors):
    file.seek(0)
    file.write(b"\x00" * 446)
    # Partition entry 0
    file.write(bytes([0]))
    # Start CHS == LBA 1
    file.write(bytes([0, 2, 0]))
    file.write(bytes([0xEE]))
    # FIXME: Add correct end CHS here
    write_chs(file, 1 + num_sectors)
    # Start LBA
    file.write((1).to_bytes(4, byteorder="little"))
    file.write(num_sectors.to_bytes(4, byteorder="little"))

    file.write(b"\x00" * 16 * 3)
    file.write((0xAA55).to_bytes(2, byteorder="little"))

with open(OUT, "wb") as file:
    make_protective_mbr(file, 1)