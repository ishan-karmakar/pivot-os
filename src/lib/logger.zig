const std = @import("std");
const drivers = @import("kernel").drivers;

const Writer = std.io.GenericWriter(void, anyerror, kernelWrite);
const writer = Writer{ .context = {} };

pub fn logger(comptime level: std.log.Level, comptime scope: @Type(.EnumLiteral), comptime format: []const u8, args: anytype) void {
    const levelText = comptime level.asText();
    const prefix = if (scope == .default) ": " else "(" ++ @tagName(scope) ++ "): ";
    std.fmt.format(writer, levelText ++ prefix ++ format ++ "\n", args) catch {};
}

fn kernelWrite(_: void, bytes: []const u8) !usize {
    _ = try drivers.qemu.write(bytes);
    return drivers.term.write(bytes);
    // return drivers.qemu.write(bytes);
}