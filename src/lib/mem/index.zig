const kernel = @import("kernel");
pub const pmm = @import("pmm.zig");
pub const Mapper = @import("mapper.zig");
pub const VMM = @import("vmm.zig");
const limine = @import("limine");
const std = @import("std");
const log = std.log.scoped(.mem);
const FixedBufferAllocator = std.heap.FixedBufferAllocator;

pub var kmapper: Mapper = undefined;
pub var kvmm: VMM = undefined;
pub var kheap: FixedBufferAllocator = undefined;

export var MMAP_REQUEST: limine.MemoryMapRequest = .{};
export var HHDM_REQUEST: limine.HhdmRequest = .{};
export var PAGING_REQUEST: limine.PagingModeRequest = .{
    .mode = .four_level,
    .flags = 0,
};

const KHEAP_SIZE = 0x1000 * 256;

// IDT isn't strictly necessary, but we would like to get paging errors instead of a triple fault
pub var PMMTask = kernel.Task{
    .name = "Physical Memory Manager",
    .init = pmm_init,
    .dependencies = &.{
        .{ .task = &kernel.drivers.idt.Task },
    },
};

pub var KMapperTask = kernel.Task{
    .name = "Kernel Mapper",
    .init = mapper_init,
    .dependencies = &.{
        .{ .task = &PMMTask },
    },
};

pub var KVMMTask = kernel.Task{
    .name = "Kernel Virtual Memory Manager",
    .init = vmm_init,
    .dependencies = &.{
        .{ .task = &PMMTask },
        .{ .task = &KMapperTask },
    },
};

pub var KHeapTask = kernel.Task{
    .name = "Kernel Heap",
    .init = kheap_init,
    .dependencies = &.{
        .{ .task = &KVMMTask },
    },
};

fn pmm_init() bool {
    if (MMAP_REQUEST.response == null or HHDM_REQUEST.response == null) return false;
    var free_mem: usize = 0;
    for (MMAP_REQUEST.response.?.entries()) |ent| {
        log.debug("start: 0x{x}, length: 0x{x}, kind: {}", .{ ent.base, ent.length, ent.kind });
        if (ent.kind == .usable) {
            free_mem += ent.length;
            pmm.add_region(ent.base, @divFloor(ent.length, 0x1000));
        }
    }
    log.debug("Found 0x{x} bytes of usable memory", .{free_mem});
    return true;
}

fn mapper_init() bool {
    if (PAGING_REQUEST.response == null or PAGING_REQUEST.response.?.mode != PAGING_REQUEST.mode) return false;
    kmapper = Mapper.create(virt(asm volatile ("mov %%cr3, %[result]"
        : [result] "=r" (-> usize),
    ) & 0xfffffffffffffffe));
    return true;
}

fn vmm_init() bool {
    const mmap = MMAP_REQUEST.response.?.entries();
    const last = mmap[mmap.len - 1];
    kvmm = VMM.create(virt(0) + last.base + last.length, KHEAP_SIZE, 0b10 | (1 << 63), &kmapper);
    return true;
}

fn kheap_init() bool {
    kheap = FixedBufferAllocator.init(kvmm.allocator().alloc(u8, KHEAP_SIZE) catch return false);
    return true;
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
