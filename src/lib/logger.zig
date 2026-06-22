const kernel = @import("root");
const std = @import("std");
const config = @import("config");

var lock: kernel.lib.Spinlock = .{};

var writer = std.Io.Writer{
    .vtable = &.{ .drain = kernel_drain },
    .buffer = &.{},
};

pub fn logger(comptime level: std.log.Level, comptime scope: @EnumLiteral(), comptime format: []const u8, args: anytype) void {
    const levelText = comptime level.asText();
    const prefix = if (scope == .default) ": " else "(" ++ @tagName(scope) ++ "): ";
    const id: u32 = kernel.cpu.smp.cpu_info(null).id;
    lock.acquire();
    defer lock.release();
    writer.print("[{}]" ++ " " ++ levelText ++ prefix ++ format ++ "\r\n", .{id} ++ args) catch {};
}

fn kernel_drain(_: *std.Io.Writer, data: []const []const u8, splat: usize) std.Io.Writer.Error!usize {
    var total: usize = 0;
    for (data[0 .. data.len - 1]) |bytes| {
        if (config.qemu) kernel.drivers.qemu.write(bytes);
        kernel.drivers.fb.write(bytes) catch {};
        total += bytes.len;
    }

    const pattern = data[data.len - 1];
    for (0..splat) |_| {
        if (config.qemu) kernel.drivers.qemu.write(pattern);
        kernel.drivers.fb.write(pattern) catch {};
        total += pattern.len;
    }

    return total;
}

export fn _putchar(char: u8) void {
    kernel.drivers.fb.write(&.{char}) catch {};
}

pub fn already_initialized(comptime log: anytype, name: []const u8) void {
    log.debug("{s} already initialized, skipping attempted initialization", .{name});
}

pub fn successfully_initialized(comptime log: anytype, name: []const u8) void {
    log.info("{s} successfully initialized", .{name});
}

pub fn failed_initialization(comptime log: anytype, name: []const u8, err: anytype) @TypeOf(err) {
    log.err("{s} failed initialization: {}", .{ name, err });
    return err;
}
