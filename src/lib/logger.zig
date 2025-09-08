const kernel = @import("root");
const std = @import("std");
const config = @import("config");

var lock = std.atomic.Value(bool).init(false);

var writer = std.Io.Writer{
    .vtable = &.{ .drain = kernelDrain },
    .buffer = &.{},
};

pub fn logger(comptime level: std.log.Level, comptime scope: @Type(.enum_literal), comptime format: []const u8, args: anytype) void {
    const levelText = comptime level.asText();
    const prefix = if (scope == .default) ": " else "(" ++ @tagName(scope) ++ "): ";
    while (lock.cmpxchgWeak(false, true, .acquire, .monotonic) != null) {}
    writer.print(levelText ++ prefix ++ format ++ "\n", args) catch {};
    lock.store(false, .release);
}

fn kernelDrain(_: *std.Io.Writer, data: []const []const u8, splat: usize) std.Io.Writer.Error!usize {
    var total: usize = 0;
    for (data[0 .. data.len - 1]) |bytes| {
        if (config.qemu) kernel.drivers.qemu.write(bytes);
        kernel.drivers.fb.write(bytes);
        total += bytes.len;
    }

    const pattern = data[data.len - 1];
    for (0..splat) |_| {
        if (config.qemu) kernel.drivers.qemu.write(pattern);
        kernel.drivers.fb.write(pattern);
        total += pattern.len;
    }

    return total;
}
