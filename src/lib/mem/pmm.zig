const limine = @import("limine");
const log = @import("std").log.scoped(.pmm);
const mem = @import("kernel").lib.mem;

const FreeRegion = struct {
    const Self = @This();
    num_pages: usize,
    next: ?*Self,

    pub fn frame(self: *Self) usize {
        self.num_pages -= 1;
        return mem.phys(@intFromPtr(self)) + self.num_pages * 0x1000;
    }
};

// FIXME: mutex

var head_region: ?*FreeRegion = null;

/// Adds region (start and number of pages) to free regions linked list
pub fn add_region(start: usize, num_pages: usize) void {
    var region: *FreeRegion = @ptrFromInt(mem.virt(start));
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
    log.debug("Allocated frame 0x{x}", .{frm});
    return frm;
}

/// Free a frame allocated with frame()
/// Takes in the PHYSICAL address
/// Runs in O(1) time
pub fn free(frm: usize) void {
    log.debug("Freeing frame 0x{x}", .{frm});
    add_region(frm, 1);
}
