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
    drivers.acpi.init_tables();
    drivers.ioapic.init();
    asm volatile ("sti");
    drivers.timers.hpet.init();
    log.info("Sleeping", .{});
    drivers.timers.hpet.nsleep(1_000_000_000);
    log.info("Done sleeping", .{});
    // drivers.timers.hpet.init();
    // drivers.lapic.bsp_init();
    // drivers.timers.lapic.calibrate();
    // drivers.smp.init();
    // lib.scheduler.init();
    // drivers.timers.lapic.start(5);
    // const proc = lib.mem.kheap.allocator().create(lib.Process) catch @panic("OOM");
    // const stack = lib.mem.kheap.allocator().alloc(u8, 0x1000) catch @panic("OOM");
    // proc.* = .{
    //     .mapper = lib.mem.kmapper,
    //     .priority = 100,
    //     .vmm = null,
    //     .stack = stack,
    //     .ef = .{ .iret_status = .{
    //         .cs = 0x8,
    //         .ss = 0x10,
    //         .rip = @intFromPtr(&kmain),
    //         .rsp = @intFromPtr(stack.ptr) + 0x1000,
    //     } },
    // };
    // lib.scheduler.queue(proc);
    // drivers.smp.cpu_info(null).cur_proc.?.status = .dead;
    // lib.scheduler.yield();
    while (true) {}
}

fn kmain() noreturn {
    log.info("Entered main kernel thread", .{});
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
