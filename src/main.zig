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

pub const TaskDep = struct {
    task: *Task,
    accept_failure: bool = false,
};

pub const Task = struct {
    /// Human readable name of task
    name: []const u8,
    /// Initialize function that returns true for success, false for failure
    init: ?*const fn () bool,
    dependencies: []const TaskDep,
    ret: ?bool = null,

    pub fn run(self: *@This()) void {
        if (self.ret != null) return;

        for (self.dependencies) |dep| {
            dep.task.run();
            if (!dep.task.ret.? and !dep.accept_failure) {
                log.err("Task \"{s}\" depends on task \"{s}\" (failed init)", .{ self.name, dep.task.name });
                @panic("Panicking...");
            }
        }

        self.ret = if (self.init) |init| init() else true;
        if (self.ret.?) {
            log.info("Task \"{s}\" completed initialization", .{self.name});
        } else {
            log.warn("Task \"{s}\" failed initialization", .{self.name});
        }
    }
};

export var LIMINE_BASE_REVISION: limine.BaseRevision = .{ .revision = 3 };

export fn _start() noreturn {
    if (!LIMINE_BASE_REVISION.is_supported()) {
        @panic("Limine bootloader base revision not supported");
    }
    drivers.term.Task.run();
    drivers.timers.Task.run();
    while (true) {}
}

pub fn panic(msg: []const u8, stacktrace: ?*std.builtin.StackTrace, _: ?usize) noreturn {
    std.log.err("{s}", .{msg});
    // TODO: Untested
    if (stacktrace) |st| {
        st.format("", .{}, lib.logger.writer) catch std.log.err("Error printing stacktrace");
    }
    asm volatile (
        \\cli
        \\hlt
    );
    unreachable;
}
