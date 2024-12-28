pub const lib = @import("lib/index.zig");
pub const drivers = @import("drivers/index.zig");
const std = @import("std");
const limine = @import("limine");
const log = std.log.scoped(.main);
const config = @import("config");

// TODO: Support options for logging for each service
// Ex. Only log PMM but not VMM, etc.

pub const std_options = .{
    .logFn = lib.logger.logger,
    .log_level = .debug,
};

export var LIMINE_BASE_REVISION: limine.BaseRevision = .{ .revision = 3 };

export fn _start() noreturn {
    if (!LIMINE_BASE_REVISION.is_supported()) {
        @panic("Limine bootloader base revision not supported");
    }
    drivers.gdt.initEarly();
    drivers.idt.init();
    lib.mem.init();
    drivers.term.init();
    drivers.gdt.initLate();
    drivers.acpi.init_tables();
    drivers.lapic.bsp_init();
    drivers.intctrl.init();
    asm volatile ("sti");
    drivers.timers.init();
    // log.info("Sleep start", .{});
    // drivers.timers.hpet.vtable.sleep(1000_000_000);
    // log.info("Sleep end", .{});
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
