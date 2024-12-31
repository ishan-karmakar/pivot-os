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
    // drivers.timers.init();
    drivers.pci.init();
    while (true) {}
}

pub fn panic(msg: []const u8, stacktrace: ?*std.builtin.StackTrace, _: ?usize) noreturn {
    std.log.err("{s}", .{msg});
    if (stacktrace) |st| {
        st.format("", .{}, lib.logger.writer) catch std.log.err("Error printing stacktrace");
    }
    asm volatile (
        \\cli
        \\hlt
    );
    unreachable;
}
