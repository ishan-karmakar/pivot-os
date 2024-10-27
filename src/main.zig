const std = @import("std");
const limine = @import("limine");
const log = std.log.scoped(.main);
pub const drivers = @import("drivers/index.zig");
pub const lib = @import("lib/index.zig");

pub const std_options = .{
    .logFn = lib.logger.logger,
};

export var LIMINE_BASE_REVISION: limine.BaseRevision = .{ .revision = 2 };

export fn _start() void {
    if (!LIMINE_BASE_REVISION.is_supported()) {
        @panic("Limine bootloader base revision not supported");
    }
    log.info("Entered kernel, starting initialization", .{});
    lib.cpu.init();
    lib.mem.init();

    while (true) {}
}

pub fn panic(msg: []const u8, _: ?*std.builtin.StackTrace, _: ?usize) noreturn {
    std.log.err("{s}", .{msg});
    asm volatile (
        \\cli
        \\hlt
    );
    unreachable;
}
