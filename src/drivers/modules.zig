const limine = @import("limine");
const kernel = @import("kernel");
const mem = @import("std").mem;

export var MODULE_REQUEST = limine.ModuleRequest{ .revision = 3 };

pub var Task = kernel.Task{
    .name = "Limine Modules",
    .init = init,
    .dependencies = &.{},
};

fn init() bool {
    return MODULE_REQUEST.response != null;
}

pub fn get_module(cmdline: []const u8) usize {
    for (MODULE_REQUEST.response.?.modules()) |m| {
        if (mem.eql(u8, mem.span(m.cmdline), cmdline)) return @intFromPtr(m.address);
    }
    @panic("Module not found");
}
