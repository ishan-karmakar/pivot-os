pub const lib = @import("lib/index.zig");
pub const drivers = @import("drivers/index.zig");
const std = @import("std");
const limine = @import("limine");
const log = std.log.scoped(.main);
const config = @import("config");

pub const std_options = .{
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

export var LIMINE_BASE_REVISION = limine.BaseRevision{ .revision = 3 };

export fn _start() noreturn {
    if (!LIMINE_BASE_REVISION.is_supported()) {
        @panic("Limine bootloader base revision not supported");
    }
    drivers.fb.Task.run();
    if (drivers.fb.Task.ret != .success) @panic("Framebuffer failed to initialize");
    lib.scheduler.Task.run();
    const stack = lib.mem.kvmm.allocator().alloc(u8, 0x1000) catch @panic("OOM");
    lib.scheduler.enqueue(lib.scheduler.create_thread(.{
        .ef = .{ .iret_status = .{
            .cs = 0x8,
            .ss = 0x10,
            .rip = @intFromPtr(&test_thread),
            .rsp = @intFromPtr(stack.ptr) + 0x1000,
        } },
        .mapper = lib.mem.kmapper,
        .priority = 99,
    }));
    // drivers.acpi.DriversTask.run();
    while (true) asm volatile ("hlt");
}

fn test_thread() noreturn {
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
