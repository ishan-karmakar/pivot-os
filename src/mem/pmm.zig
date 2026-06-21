const kernel = @import("root");
const limine = @import("limine");
const std = @import("std");
const log = @import("std").log.scoped(.pmm);
const mem = kernel.mem;

const FreeRegion = struct {
    const Self = @This();
    num_pages: usize,
    next: ?*Self,

    pub fn frame(self: *Self) usize {
        self.num_pages -= 1;
        return mem.phys(@intFromPtr(self)) + self.num_pages * 0x1000;
    }
};

var mutex = std.atomic.Value(bool).init(false);
var head_region: ?*FreeRegion = null;
var initialized = false;

pub fn init() !void {
    if (initialized)
        return kernel.lib.logger.already_initialized(log, "PMM");
    kernel.cpu.idt.init_bsp();

    if (mem.HHDM_REQUEST.response == null)
        return kernel.lib.logger.failed_initialization(log, "PMM", error.HHDMUnavailable);

    const response: *limine.limine_memmap_response = mem.MMAP_REQUEST.response orelse
        return kernel.lib.logger.failed_initialization(log, "PMM", error.MMAPUnavailable);

    for (0..response.entry_count) |i| {
        const entry: *limine.limine_memmap_entry = response.entries[i];
        if (entry.type == limine.LIMINE_MEMMAP_USABLE)
            add_region(entry.base, entry.length / 0x1000);
    }
    initialized = true;
    kernel.lib.logger.successfully_initialized(log, "PMM");
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
    var size: usize = 0;
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
    const hregion = head_region orelse @panic("No region has been added yet");
    const frm = hregion.frame();
    if (hregion.num_pages == 0) {
        head_region = hregion.next;
    }
    return frm;
}

pub fn frames(num_pages: usize) usize {
    while (mutex.cmpxchgWeak(false, true, .acquire, .monotonic) != null) {}
    defer mutex.store(false, .release);
    var prev_region: ?*FreeRegion = null;
    var region = head_region orelse @panic("No region has been added yet");
    while (true) : ({
        prev_region = region;
        region = region.next orelse @panic("No regions are big enough to satisfy request");
    }) {
        if (region.num_pages >= num_pages) {
            region.num_pages -= num_pages - 1;
            const frm = region.frame();
            if (region.num_pages == 0) {
                if (prev_region) |pr| {
                    pr.next = region.next;
                } else if (head_region.? == region)
                    head_region = region.next;
            }
            return frm;
        }
    }
}

/// Free a frame allocated with frame()
/// Takes in the PHYSICAL address
/// Runs in O(1) time
pub inline fn free(frm: usize) void {
    add_region(frm, 1);
}
