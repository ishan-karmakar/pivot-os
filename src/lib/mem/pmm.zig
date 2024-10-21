const limine = @import("limine");
const log = @import("std").log.scoped(.pmm);

const FreeRegion = struct {
    const Self = @This();
    num_pages: usize,
    next: *Self,

    pub fn frame(self: *Self) usize {
        self.num_pages -= 1;
        return phys(@intFromPtr(self)) + self.num_pages * 0x1000;
    }
};

export var MMAP_REQUEST: limine.MemoryMapRequest = .{};
export var HHDM_REQUEST: limine.HhdmRequest = .{};
export var PAGING_REQUEST: limine.PagingModeRequest = .{
    .mode = .four_level,
    .flags = 0,
};

var head_region: ?*FreeRegion = null;

pub fn init() void {
    if (HHDM_REQUEST.response == null) {
        @panic("Limine HHDM response is null");
    }
    if (PAGING_REQUEST.response == null or PAGING_REQUEST.response.?.mode != .four_level) {
        @panic("Limine paging response is either null or not set to our preferred mode");
    }
    const response: *limine.MemoryMapResponse = MMAP_REQUEST.response orelse @panic("Limine memory map response is null");
    for (response.entries()) |ent| {
        log.debug("start: 0x{x}, length: 0x{x}, kind: {}", .{ ent.base, ent.length, ent.kind });
    }
}

fn add_region(start: usize, size: usize) void {
    var region = 
}

/// Reserve a frame from physical memory
/// The PHYSICAL address is returned
pub fn frame() usize {}

pub inline fn phys(addr: usize) usize {
    return addr - HHDM_REQUEST.response.?.offset;
}

pub inline fn virt(addr: usize) usize {
    return addr + HHDM_REQUEST.response.?.offset;
}
