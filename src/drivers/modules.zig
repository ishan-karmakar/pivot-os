const limine = @import("limine");
const kernel = @import("kernel");
const mem = @import("std").mem;

export var MODULE_REQUEST = limine.ModuleRequest{ .revision = 3 };

pub var Task = kernel.Task{
    .name = "Limine Modules",
    .init = init,
    .dependencies = &.{},
};

fn init() kernel.Task.Ret {
    return if (MODULE_REQUEST.response != null) .success else .failed;
}

pub fn get_module(cmdline: []const u8) usize {
    for (MODULE_REQUEST.response.?.getModules()) |m| {
        if (mem.eql(u8, mem.span(m.string), cmdline)) return @intFromPtr(m.address);
    }
    @panic("Module not found");
}
