const std = @import("std");
const logger = @import("lib/logger.zig");
const gdt = @import("drivers/gdt.zig");
const idt = @import("drivers/idt.zig");

pub const std_options = .{
    .log_level = .debug,
    .logFn = logger.kernelLog,
};

export fn _start() void {
    std.log.info("Entered kernel, starting initialization", .{});
    gdt.init_static();
    idt.init();

    while (true) {}
}
