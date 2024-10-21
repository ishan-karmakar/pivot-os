const std = @import("std");
const limine = @import("limine");
const log = std.log.scoped(.main);
pub const services = @import("services/index.zig");
pub const drivers = @import("drivers/index.zig");
pub const lib = @import("lib/index.zig");

pub const std_options = .{
    .logFn = services.logger.logger,
};

export var LIMINE_BASE_REVISION: limine.BaseRevision = .{ .revision = 2 };

export fn _start() void {
    if (!LIMINE_BASE_REVISION.is_supported()) {
        @panic("Limine bootloader base revision not supported");
    }
    log.info("Entered kernel, starting initialization", .{});
    services.cpu.init();
    services.mem.init();
    log.info("{x}", .{lib.mem.pmm.frame()});

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
