const std = @import("std");
const limine = @import("limine");
const config = @import("config");
const log = std.log.scoped(.main);
pub const drivers = @import("drivers/index.zig");
pub const lib = @import("lib/index.zig");

// TODO: Support options for logging for each service
// Ex. Only log PMM but not VMM, etc.

pub const std_options = .{
    .logFn = lib.logger.logger,
};

export var LIMINE_BASE_REVISION: limine.BaseRevision = .{ .revision = 3 };

export fn _start() void {
    if (comptime config.debug) asm volatile ("1: jmp 1b");
    if (!LIMINE_BASE_REVISION.is_supported()) {
        @panic("Limine bootloader base revision not supported");
    }
    log.info("Entered kernel, starting initialization", .{});
    drivers.gdt.init_static();
    drivers.idt.init();
    lib.mem.init();
    drivers.gdt.init_dyn();
    drivers.term.init();
    drivers.pic.init();
    drivers.timers.pit.init();
    drivers.lapic.bsp_init();
    drivers.timers.lapic.calibrate();
    // drivers.timers.lapic.start(1);
    drivers.acpi.init();
    drivers.ioapic.init();
    drivers.acpi.init2();

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
