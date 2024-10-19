const std = @import("std");
const logger = @import("lib/logger.zig");

pub const std_options = .{
    .log_level = .debug,
    .logFn = logger.kernelLog,
};

export fn _start() void {
    std.log.info("Entered kernel, starting initialization", .{});

    while (true) {}
}
