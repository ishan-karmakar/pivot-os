pub const lib = @import("lib/index.zig");
pub const drivers = @import("drivers/index.zig");
const std = @import("std");
const limine = @import("limine");
const log = std.log.scoped(.main);

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

export fn _start() noreturn {
    if (!limine.LIMINE_LOADED_BASE_REVISION_VALID(LIMINE_BASE_REVISION)) {
        @panic("Limine bootloader base revision not valid");
    } else if (!limine.LIMINE_BASE_REVISION_SUPPORTED(LIMINE_BASE_REVISION)) {
        @panic("Limine bootloader base revision not supported");
    }
    drivers.fb.init() catch @panic("");

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
