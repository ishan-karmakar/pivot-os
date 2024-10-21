const limine = @import("limine");
const log = @import("std").log.scoped(.pmm);

const FreeRegion = struct {
    const Self = @This();
    num_pages: usize,
    next: ?*Self,

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
    if (PAGING_REQUEST.response == null or PAGING_REQUEST.response.?.mode != PAGING_REQUEST.mode) {
        @panic("Limine paging response is either null or not set to our preferred mode");
    }
    const response: *limine.MemoryMapResponse = MMAP_REQUEST.response orelse @panic("Limine memory map response is null");
    for (response.entries()) |ent| {
        log.debug("start: 0x{x}, length: 0x{x}, kind: {}", .{ ent.base, ent.length, ent.kind });
        if (ent.kind == .usable) {
            add_region(ent.base, @divFloor(ent.length, 0x1000));
        }
    }
    log.info("Initialized physical memory manager", .{});
}

/// Adds region (start and number of pages) to free regions linked list
fn add_region(start: usize, num_pages: usize) void {
    var region: *FreeRegion = @ptrFromInt(virt(start));
    region.num_pages = num_pages;
    region.next = head_region;
    head_region = region;
}

/// Reserve a frame from physical memory
/// The PHYSICAL address is returned
/// Runs in O(1) time
pub fn frame() usize {
    const hregion: *FreeRegion = head_region orelse @panic("No region has been added yet");
    const frm = hregion.frame();
    if (hregion.num_pages == 0) {
        head_region = hregion.next;
    }
    return frm;
}

/// Free a frame allocated with frame()
/// Takes in the PHYSICAL address
/// Runs in O(1) time
pub fn free(frm: usize) void {
    add_region(frm, 1);
}

/// Converts physical address to virtual address
/// DOES NOT CHECK FOR OVERFLOW
pub inline fn phys(addr: usize) usize {
    return addr - HHDM_REQUEST.response.?.offset;
}

/// Converts virtual address to physical address
/// DOES NOT CHECK FOR OVERFLOW
pub inline fn virt(addr: usize) usize {
    return addr + HHDM_REQUEST.response.?.offset;
}
