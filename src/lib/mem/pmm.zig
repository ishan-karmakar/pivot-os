const kernel = @import("kernel");
const limine = @import("limine");
const std = @import("std");
const log = @import("std").log.scoped(.pmm);
const mem = kernel.lib.mem;

const FreeRegion = struct {
    const Self = @This();
    num_pages: usize,
    next: ?*Self,

    pub fn frame(self: *Self) usize {
        self.num_pages -= 1;
        return mem.phys(@intFromPtr(self)) + self.num_pages * 0x1000;
    }
};

// IDT isn't strictly necessary, but we would like to get paging errors instead of a triple fault
pub var Task = kernel.Task{
    .name = "Physical Memory Manager",
    .init = init,
    .dependencies = &.{
        .{ .task = &kernel.drivers.idt.Task },
    },
};

var mutex = std.atomic.Value(bool).init(false);
var head_region: ?*FreeRegion = null;

fn init() kernel.Task.Ret {
    if (mem.HHDM_REQUEST.response == null) return .failed;
    const response = mem.MMAP_REQUEST.response orelse return .failed;
    for (response.entries()) |ent| {
        // log.debug("start: 0x{x}, length: 0x{x}, kind: {}", .{ ent.base, ent.length, ent.kind });
        if (ent.kind == .usable) {
            add_region(ent.base, ent.length / 0x1000);
        }
    }
    return .success;
}

/// Adds region (start and number of pages) to free regions linked list
pub fn add_region(start: usize, num_pages: usize) void {
    while (mutex.cmpxchgWeak(false, true, .acquire, .monotonic) != null) {}
    defer mutex.store(false, .release);
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
    while (mutex.cmpxchgWeak(false, true, .acquire, .monotonic) != null) {}
    defer mutex.store(false, .release);
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
