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
    .log_level = .debug,
};

export var LIMINE_BASE_REVISION: limine.BaseRevision = .{ .revision = 3 };

export fn _start() noreturn {
    if (comptime config.debug) asm volatile ("1: jmp 1b");
    if (!LIMINE_BASE_REVISION.is_supported()) {
        @panic("Limine bootloader base revision not supported");
    }
    drivers.gdt.init_static();
    drivers.idt.init();
    lib.mem.init();
    drivers.term.init();
    drivers.gdt.init_dyn();
    drivers.pic.init();
    asm volatile ("sti");
    drivers.timers.pit.init();
    drivers.lapic.bsp_init();
    drivers.timers.lapic.calibrate();
    drivers.smp.init();
    lib.scheduler.init();
    const thread2_proc = lib.mem.kheap.allocator().create(lib.Process) catch @panic("OOM");
    thread2_proc.* = .{
        .ef = .{ .iret_status = .{
            .cs = 0x8,
            .ss = 0x10,
            .rip = @intFromPtr(&thread2),
            .rsp = @intFromPtr((lib.mem.kheap.allocator().alloc(u8, 0x1000) catch @panic("OOM")).ptr) + 0x1000,
        } },
        .mapper = lib.mem.kmapper,
        .next = null,
    };
    lib.scheduler.queue(thread2_proc);
    log.info("test1", .{});
    asm volatile ("int $0x20");
    log.info("test3", .{});
    while (true) {}
}

fn thread2() noreturn {
    log.info("test2", .{});
    asm volatile ("int $0x20");
    while (true) {}
}

pub fn ap_init(info: *limine.SmpInfo) callconv(.C) noreturn {
    drivers.cpu.set_kgs(info.extra_argument);
    drivers.gdt.lgdt();
    drivers.idt.lidt();
    drivers.lapic.ap_init();
    asm volatile ("sti");
    @atomicStore(bool, &drivers.smp.cpu_info(null).ready, true, .unordered);
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
