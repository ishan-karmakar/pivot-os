const limine = @import("limine");
const kernel = @import("root");
const std = @import("std");
const mem = std.mem;
const log = std.log.scoped(.modules);

export var MODULE_REQUEST = limine.limine_module_request{
    .id = kernel.LIMINE_REQUEST_ID(0x3e7e279702be32af, 0xca1c4f3bd1280cee),
    .revision = 1,
};

var initialized = false;

pub fn init() !void {
    if (initialized) return;
    if (MODULE_REQUEST.response == null)
        return kernel.lib.logger.failed_initialization(log, "Modules", error.ModulesUnavailable);
    initialized = true;
    kernel.lib.logger.successfully_initialized(log, "Modules");
}

pub fn get_module(cmdline: []const u8) !usize {
    if (!initialized) {
        log.err("get_modules() was called when modules are not initialized", .{});
        return error.ModulesNotInitialized;
    }
    for (0..MODULE_REQUEST.response.*.module_count) |i| {
        const module: *limine.limine_file = MODULE_REQUEST.response.*.modules[i];
        if (std.mem.eql(u8, std.mem.span(module.string), cmdline)) return @intFromPtr(module.address);
    }
    log.err("Failed to find module '{s}'", .{cmdline});
    return error.ModuleNotFound;
}
