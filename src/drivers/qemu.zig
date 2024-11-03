const std = @import("std");
const serial = @import("kernel").drivers.serial;

const QEMU_SERIAL_PORT = 0x3F8;

pub fn write(bytes: []const u8) !usize {
    for (bytes) |byte| {
        serial.out(QEMU_SERIAL_PORT, byte);
    }
    return bytes.len;
}
