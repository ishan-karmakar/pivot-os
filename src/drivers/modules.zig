const limine = @import("limine");
const kernel = @import("root");
const mem = @import("std").mem;

export var MODULE_REQUEST = limine.Module.Request{};

pub var Task = kernel.Task{
    .name = "Limine Modules",
    .init = init,
    .dependencies = &.{},
};

fn init() kernel.Task.Ret {
    return if (MODULE_REQUEST.response != null) .success else .failed;
}

pub fn get_module(cmdline: []const u8) usize {
    for (MODULE_REQUEST.response.?.get_modules()) |m|
        if (mem.eql(u8, mem.span(m.string), cmdline)) return @intFromPtr(m.address);
    @panic("Module not found");
}
