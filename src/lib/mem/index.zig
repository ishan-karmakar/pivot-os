pub const pmm = @import("pmm.zig");
pub const Mapper = @import("mapper.zig");
pub const VMM = @import("vmm.zig");
const limine = @import("limine");
const log = @import("std").log.scoped(.mem);

pub var kmapper: Mapper = undefined;
pub var kvmm: VMM = undefined;

export var MMAP_REQUEST: limine.MemoryMapRequest = .{};
export var HHDM_REQUEST: limine.HhdmRequest = .{};
export var PAGING_REQUEST: limine.PagingModeRequest = .{
    .mode = .four_level,
    .flags = 0,
};

pub fn init() void {
    if (HHDM_REQUEST.response == null) {
        @panic("Limine HHDM response is null");
    }
    if (PAGING_REQUEST.response == null or PAGING_REQUEST.response.?.mode != PAGING_REQUEST.mode) {
        @panic("Limine paging response is either null or not set to our preferred mode");
    }
    if (MMAP_REQUEST.response == null) {
        @panic("Limine memory map response is null");
    }

    const free_mem = pmm_init();
    mapper_init();
    vmm_init(free_mem);
    _ = kvmm.alloc(0x1000, 0, 0);
    _ = kvmm.alloc(0x2000, 0, 0);
    kvmm.free((kvmm.alloc(0x3000, 0, 0) orelse @panic("alloc failed"))[0..0x3000], 0, 0);

    // const allocator = kvmm.allocator();
    // log.info("{}", .{allocator});
}

fn pmm_init() usize {
    var free_mem: usize = 0;
    for (MMAP_REQUEST.response.?.entries()) |ent| {
        log.debug("start: 0x{x}, length: 0x{x}, kind: {}", .{ ent.base, ent.length, ent.kind });
        if (ent.kind == .usable) {
            free_mem += ent.length;
            pmm.add_region(ent.base, @divFloor(ent.length, 0x1000));
        }
    }
    log.info("Found 0x{x} bytes of usable memory", .{free_mem});
    return free_mem;
}

fn mapper_init() void {
    kmapper = Mapper.create(virt(asm volatile ("mov %%cr3, %[result]"
        : [result] "=r" (-> usize),
    ) & 0xfffffffffffffffe));
}

fn vmm_init(free_mem: usize) void {
    const mmap = MMAP_REQUEST.response.?.entries();
    const last = mmap[mmap.len - 1];
    // FIXME: I don't think we need ALL of free memory for kernel VMM, maybe constant limit
    kvmm = VMM.create(virt(0) + last.base + last.length, free_mem, 0b10 | (1 << 63), &kmapper);
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
