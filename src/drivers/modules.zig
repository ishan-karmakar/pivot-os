const limine = @import("limine");
const kernel = @import("root");
const mem = @import("std").mem;

export var MODULE_REQUEST = limine.limine_module_request{
    .id = kernel.LIMINE_REQUEST_ID(0x3e7e279702be32af, 0xca1c4f3bd1280cee),
    .revision = 1,
};

pub var Task = kernel.Task{
    .name = "Limine Modules",
    .init = init,
    .dependencies = &.{},
};

fn init() kernel.Task.Ret {
    return if (MODULE_REQUEST.response != null) .success else .failed;
}

pub fn get_module(cmdline: []const u8) usize {
    for (0..MODULE_REQUEST.response.*.module_count) |i| {
        const module: *limine.limine_file = MODULE_REQUEST.response.*.modules[i];
        if (mem.eql(u8, mem.span(module.string), cmdline)) return @intFromPtr(module.address);
    }
    @panic("Module not found");
}
