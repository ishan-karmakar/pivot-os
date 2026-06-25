const kernel = @import("root");
const std = @import("std");
const config = @import("config");

var lock: kernel.lib.Spinlock = .{};

pub var writers = std.ArrayList(std.Io.Writer).empty;

pub fn logger(comptime level: std.log.Level, comptime scope: @EnumLiteral(), comptime format: []const u8, args: anytype) void {
    const levelText = comptime level.asText();
    const prefix = if (scope == .default) ": " else "(" ++ @tagName(scope) ++ "): ";
    const id: u32 = kernel.cpu.smp.cpu_info(null).id;
    lock.acquire();
    defer lock.release();

    for (writers.items) |*writer| {
        writer.print("[{}] ", .{id}) catch {};
        switch (level) {
            .debug => kernel.drivers.fb.set_fg(5, true) catch {},
            .info => kernel.drivers.fb.set_fg(2, false) catch {},
            .warn => kernel.drivers.fb.set_fg(3, true) catch {},
            .err => kernel.drivers.fb.set_fg(1, false) catch {},
        }
        writer.print(levelText, .{}) catch {};
        kernel.drivers.fb.set_fg(7, true) catch {};
        writer.print(prefix, .{}) catch {};
        kernel.drivers.fb.reset_fg() catch {};
        writer.print(format ++ "\r\n", args) catch {};
    }
}

export fn _putchar(char: u8) void {
    _ = char;
    // kernel.drivers.fb.write(&.{char}) catch {};
}

pub fn successfully_initialized(comptime log: anytype, name: []const u8) void {
    log.info("{s} successfully initialized", .{name});
}

pub fn failed_initialization(comptime log: anytype, name: []const u8, err: anytype) @TypeOf(err) {
    log.err("{s} failed initialization: {}", .{ name, err });
    return err;
}
