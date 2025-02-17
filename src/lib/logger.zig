const kernel = @import("kernel");
const std = @import("std");
const config = @import("config");

const Writer = std.io.GenericWriter(void, anyerror, kernelWrite);
pub const writer = Writer{ .context = {} };
var lock = std.atomic.Value(bool).init(false);

pub fn logger(comptime level: std.log.Level, comptime scope: @Type(.enum_literal), comptime format: []const u8, args: anytype) void {
    const levelText = comptime level.asText();
    const prefix = if (scope == .default) ": " else "(" ++ @tagName(scope) ++ "): ";
    while (lock.cmpxchgWeak(false, true, .acquire, .monotonic) != null) {}
    std.fmt.format(writer, levelText ++ prefix ++ format ++ "\n", args) catch {};
    lock.store(false, .release);
}

fn kernelWrite(_: void, bytes: []const u8) !usize {
    if (config.qemu) kernel.drivers.qemu.write(bytes);
    kernel.drivers.fb.write(bytes);
    return bytes.len;
}
