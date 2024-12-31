const kernel = @import("kernel");
const limine = @import("limine");
const log = @import("std").log.scoped(.pmm);
const mem = kernel.lib.mem;
const Mutex = kernel.lib.Mutex;

const FreeRegion = struct {
    const Self = @This();
    num_pages: usize,
    next: ?*Self,

    pub fn frame(self: *Self) usize {
        self.num_pages -= 1;
        return mem.phys(@intFromPtr(self)) + self.num_pages * 0x1000;
    }
};

var mutex = Mutex{};
var head_region: ?*FreeRegion = null;

/// Adds region (start and number of pages) to free regions linked list
pub fn add_region(start: usize, num_pages: usize) void {
    mutex.lock();
    defer mutex.unlock();
    var region: *FreeRegion = @ptrFromInt(mem.virt(start));
    region.num_pages = num_pages;
    region.next = head_region;
    head_region = region;
}

/// Returns total amount of free space
/// Runs in O(N), so should rarely be used
pub fn get_free_size() usize {
    var cur = head_region;
    var size = 0;
    while (cur != null) {
        size += cur.?.num_pages * 0x1000;
        cur = cur.?.next;
    }
    return size;
}

/// Reserve a frame from physical memory
/// The PHYSICAL address is returned
/// Runs in O(1) time
pub fn frame() usize {
    mutex.lock();
    defer mutex.unlock();
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
pub inline fn free(frm: usize) void {
    add_region(frm, 1);
}
