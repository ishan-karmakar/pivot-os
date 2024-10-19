const std = @import("std");
const qemu = @import("../drivers/qemu.zig");

const Writer = std.io.GenericWriter(void, anyerror, kernelWrite);
const writer = Writer{ .context = {} };

pub fn kernelLog(comptime level: std.log.Level, comptime scope: @Type(.EnumLiteral), comptime format: []const u8, args: anytype) void {
    const levelText = comptime level.asText();
    const prefix = if (scope == .default) ": " else "(" ++ @tagName(scope) ++ "): ";
    std.fmt.format(writer, levelText ++ prefix ++ format ++ "\n", args) catch {};
}

fn kernelWrite(_: void, bytes: []const u8) !usize {
    return qemu.write(bytes);
}
