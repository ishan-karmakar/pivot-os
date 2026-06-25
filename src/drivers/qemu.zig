const std = @import("std");
const serial = @import("root").drivers.serial;
const Writer = std.Io.Writer;

const QEMU_SERIAL_PORT = 0x3F8;

var buffer: [128]u8 = undefined;
pub const writer = Writer{
    .vtable = &.{ .drain = drain },
    .buffer = &buffer,
};

fn drain(w: *Writer, data: []const []const u8, splat: usize) Writer.Error!usize {
    var total: usize = w.end;
    write(w.buffer[0..w.end]);
    w.end = 0;

    for (data) |bytes| {
        write(bytes);
        total += bytes.len;
    }

    for (0..splat) |_| {
        write(data[data.len - 1]);
        total += data[data.len - 1].len;
    }
    return total;
}

fn write(bytes: []const u8) void {
    for (bytes) |byte|
        serial.out(QEMU_SERIAL_PORT, byte);
}
