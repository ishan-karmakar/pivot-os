pub const lib = @import("lib/index.zig");
pub const drivers = @import("drivers/index.zig");
pub const cpu = @import("cpu/index.zig");
pub const timers = @import("timers/index.zig");
pub const mem = @import("mem/index.zig");
const std = @import("std");
const limine = @import("limine");
const log = std.log.scoped(.main);

comptime {
    _ = @import("thirdparty/uacpi.zig");
    _ = @import("thirdparty/lwip.zig");
}

pub const std_options = std.Options{
    .logFn = lib.logger.logger,
    .log_level = .debug,
};

pub const Task = struct {
    pub const Ret = enum {
        success,
        failed,
        skipped,
    };
    pub const Dep = struct {
        task: *Task,
        accept_failure: bool = false,
    };

    /// Human readable name of task
    name: []const u8,
    /// Initialize function that returns true for success, false for failure
    init: ?*const fn () Ret,
    dependencies: []const Dep,
    ret: ?Ret = null,

    pub fn run(self: *@This()) void {
        if (self.ret != null) return;

        for (self.dependencies) |dep| {
            dep.task.run();
            if (dep.task.ret != .success and !dep.accept_failure) {
                log.warn("Task \"{s}\" depends on task \"{s}\" (failed/skipped init)", .{ self.name, dep.task.name });
                self.ret = .skipped;
                break;
            }
        }

        const ret = self.ret orelse if (self.init) |init| init() else .success;
        self.ret = ret;

        switch (ret) {
            .success => log.info("Task \"{s}\" successfully initialized", .{self.name}),
            .skipped => log.debug("Task \"{s}\" skipped initialization", .{self.name}),
            .failed => log.err("Task \"{s}\" failed initialization", .{self.name}),
        }
    }

    pub fn reset(self: *@This()) void {
        reset_inner(self);
    }

    fn reset_inner(task: *Task) void {
        task.ret = null;
        for (task.dependencies) |dep| reset_inner(dep.task);
    }
};

pub fn LIMINE_REQUEST_ID(suffix0: u64, suffix1: u64) [4]u64 {
    return [_]u64{ 0xc7b1dd30df4c8b88, 0x0a82e883a194f07b, suffix0, suffix1 };
}

export var LIMINE_BASE_REVISION = [_]u64{ 0xf9562b2d5c95a6c8, 0x6a7b384944536bdc, 6 };

export var BOOTLOADER_INFO_REQUEST = limine.limine_bootloader_info_request{
    .id = LIMINE_REQUEST_ID(0xf55038d8e2a1202f, 0x279426fcf5f59740),
};

export var EXECUTABLE_CMDLINE_REQUEST = limine.limine_executable_cmdline_request{
    .id = LIMINE_REQUEST_ID(0x4b161536e598651e, 0xb390ad4a2f1f303a),
};

export var FIRMWARE_TYPE_REQUEST = limine.limine_firmware_type_request{
    .id = LIMINE_REQUEST_ID(0x8c2f75d90bef28a8, 0x7045a4688eac00c3),
};

export var BOOTLOADER_PERFORMANCE_REQUEST = limine.limine_bootloader_performance_request{
    .id = LIMINE_REQUEST_ID(0x6b50ad9bf36d13ad, 0xdc4c7e88fc759e17),
};

export var DATE_AT_BOOT_REQUEST = limine.limine_date_at_boot_request{
    .id = LIMINE_REQUEST_ID(0x502746e184c088aa, 0xfbc5ec83e6327893),
};

export fn _start() noreturn {
    if (!limine.LIMINE_LOADED_BASE_REVISION_VALID(LIMINE_BASE_REVISION)) {
        @panic("Limine bootloader base revision not valid");
    } else if (!limine.LIMINE_BASE_REVISION_SUPPORTED(LIMINE_BASE_REVISION)) {
        @panic("Limine bootloader base revision not supported");
    }

    cpu.smp.init_bsp() catch @panic("Failed to initialize BSP CPU information");
    log.info("Kernel initialization starting", .{});
    drivers.fb.init_main() catch {};

    if (BOOTLOADER_INFO_REQUEST.response) |res|
        log.info("Kernel was booted with {s} (version {s})", .{ std.mem.span(res.*.name), std.mem.span(res.*.version) });
    if (BOOTLOADER_PERFORMANCE_REQUEST.response) |res|
        log.info("Bootloader time of reset: {} μs, time of bootloader initialization: {} μs, time of executable handoff: {} μs", .{ res.*.reset_usec, res.*.init_usec, res.*.exec_usec });
    if (EXECUTABLE_CMDLINE_REQUEST.response) |res|
        log.info("Kernel cmdline is: {s}", .{std.mem.span(res.*.cmdline)});
    if (FIRMWARE_TYPE_REQUEST.response) |res| {
        log.info("Firmware type is: {s}", .{switch (res.*.firmware_type) {
            limine.LIMINE_FIRMWARE_TYPE_X86BIOS => "x86 BIOS",
            limine.LIMINE_FIRMWARE_TYPE_EFI32 => "32-bit EFI",
            limine.LIMINE_FIRMWARE_TYPE_EFI64 => "64-bit EFI",
            limine.LIMINE_FIRMWARE_TYPE_SBI => "SBI",
            else => "Unknown",
        }});
    }
    if (DATE_AT_BOOT_REQUEST.response) |res|
        log.info("Kernel booted at {} seconds (UNIX time)", .{res.*.timestamp});

    drivers.smbios.init() catch {};
    cpu.gdt.init_static();
    cpu.idt.init_bsp();
    mem.pmm.init() catch {};
    mem.init_kmapper() catch {};
    mem.init_kvmm() catch {};
    mem.init_kheap() catch {};
    drivers.fb.init_all() catch {};
    cpu.gdt.init_dynamic() catch {};
    drivers.acpi.init_tables() catch {};
    drivers.intctrl.init() catch {};
    cpu.scheduler.init() catch {};

    while (true) asm volatile ("hlt");
}

pub fn panic(msg: []const u8, _: ?*std.builtin.StackTrace, _: ?usize) noreturn {
    std.log.err("{s}", .{msg});
    asm volatile (
        \\cli
        \\hlt
    );
    unreachable;
}
