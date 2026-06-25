const std = @import("std");
const serial = @import("root").drivers.serial;
const Writer = std.Io.Writer;

const QEMU_SERIAL_PORT = 0x3F8;
pub const writer = Writer{
    .vtable = &.{ .drain = drain },
    .buffer = &.{},
};

fn drain(_: *Writer, data: []const []const u8, splat: usize) Writer.Error!usize {
    var total: usize = 0;
    for (data[0 .. data.len - 1]) |bytes| {
        write(bytes);
        total += bytes.len;
    }

    const pattern = data[data.len - 1];
    for (0..splat) |_| {
        write(pattern);
        total += pattern.len;
    }
    return total;
}

fn write(bytes: []const u8) void {
    for (bytes) |byte|
        serial.out(QEMU_SERIAL_PORT, byte);
}
