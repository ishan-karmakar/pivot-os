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
        const terminal = std.Io.Terminal{
            .mode = .escape_codes,
            .writer = writer,
        };
        terminal.setColor(.reset) catch {};
        terminal.setColor(.bright_white) catch {};
        writer.print("[{}] ", .{id}) catch {};
        terminal.setColor(switch (level) {
            .debug => .bright_magenta,
            .info => .green,
            .warn => .yellow,
            .err => .red,
        }) catch {};
        writer.print(levelText, .{}) catch {};
        terminal.setColor(.reset) catch {};
        terminal.setColor(.dim) catch {};
        terminal.setColor(.bold) catch {};
        writer.print(prefix, .{}) catch {};
        terminal.setColor(.reset) catch {};
        writer.print(format ++ "\r\n", args) catch {};
        writer.flush() catch {};
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
