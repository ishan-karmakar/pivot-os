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
            if (dep.task.ret.? != .success and !dep.accept_failure) {
                log.warn("Task \"{s}\" depends on task \"{s}\" (failed/skipped init)", .{ self.name, dep.task.name });
                self.ret = .skipped;
                break;
            }
        }

        const ret = self.ret orelse if (self.init) |init| init() else .success;
        self.ret = ret;

        switch (ret) {
            .success => log.info("Task \"{s}\" successfully initialized", .{self.name}),
            .skipped => log.info("Task \"{s}\" skipped initialization", .{self.name}),
            .failed => log.err("Task \"{s}\" failed initialization", .{self.name}),
        }
    }
};

export var LIMINE_BASE_REVISION = limine.BaseRevision{ .revision = 3 };

export fn _start() noreturn {
    if (!LIMINE_BASE_REVISION.is_supported()) {
        @panic("Limine bootloader base revision not supported");
    }
    drivers.fb.Task.run();
    if (drivers.fb.Task.ret.? != .success) @panic("Framebuffer failed to initialize");
    drivers.modules.Task.run();
    handle_elf(drivers.modules.get_module("kmod-ide"));
    while (true) asm volatile ("hlt");
}

const ELFHeader = extern struct {
    ident: [4]u8,
    bit: u8,
    endian: u8,
    hdr_version: u8,
    abi: u8,
    rsv: u64,
    type: u16,
    iset: u16,
    elf_version: u32,
    pentry_off: u64,
    phdr_offset: u64,
    shdr_offset: u64,
    flags: u32,
    hdr_size: u16,
    ent_size_phdr: u16,
    num_ent_phdr: u16,
    ent_size_shdr: u16,
    num_ent_shdr: u16,
    sec_idx_hdr_string: u16,
};

fn handle_elf(addr: usize) void {
    const elf: *const ELFHeader = @ptrFromInt(addr);
    if (!std.mem.eql(u8, &elf.ident, &.{ 0x7F, 'E', 'L', 'F' })) return;
    log.info("ELF is valid", .{});
}

pub fn panic(msg: []const u8, _: ?*std.builtin.StackTrace, _: ?usize) noreturn {
    std.log.err("{s}", .{msg});
    asm volatile (
        \\cli
        \\hlt
    );
    unreachable;
}
