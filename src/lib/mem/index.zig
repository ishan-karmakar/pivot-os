const kernel = @import("root");
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

pub export var MMAP_REQUEST = limine.limine_memmap_request{
    .id = kernel.LIMINE_REQUEST_ID(0x67cf3d9d378a806f, 0xe304acdfc50c3c62),
};
pub export var HHDM_REQUEST = limine.limine_hhdm_request{
    .id = kernel.LIMINE_REQUEST_ID(0x48dcf1cb8ad2b852, 0x63984e959a98244b),
};
pub export var PAGING_REQUEST = limine.limine_paging_mode_request{
    .id = kernel.LIMINE_REQUEST_ID(0x95c1a0edab0944cb, 0xa4e5cb3842f7488a),
    .revision = 1,
    .mode = limine.LIMINE_PAGING_MODE_X86_64_4LVL,
    .max_mode = limine.LIMINE_PAGING_MODE_X86_64_4LVL,
    .min_mode = limine.LIMINE_PAGING_MODE_X86_64_4LVL,
};

const KHEAP_SIZE = 0x1000 * 128;
const KVMM_SIZE = KHEAP_SIZE + 0x1000 * 2;

pub var KMapperTask = kernel.Task{
    .name = "Kernel Mapper",
    .init = mapper_init,
    .dependencies = &.{
        .{ .task = &pmm.Task },
    },
};

pub var KMapperTaskAP = kernel.Task{
    .name = "Kernel Mapper (AP)",
    .init = mapper_ap_init,
    .dependencies = &.{},
};

pub var KVMMTask = kernel.Task{
    .name = "Kernel Virtual Memory Manager",
    .init = vmm_init,
    .dependencies = &.{
        .{ .task = &pmm.Task },
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

fn mapper_init() kernel.Task.Ret {
    if (PAGING_REQUEST.response == null or PAGING_REQUEST.response.*.mode != PAGING_REQUEST.mode) return .failed;
    kmapper = Mapper.create(virt(asm volatile ("mov %%cr3, %[result]"
        : [result] "=r" (-> usize),
    ) & 0xfffffffffffffffe));
    return .success;
}

fn mapper_ap_init() kernel.Task.Ret {
    kernel.drivers.cpu.set_cr3(phys(@intFromPtr(kmapper.pml4)));
    return .success;
}

fn vmm_init() kernel.Task.Ret {
    // The VMM size will be a percentage of the total free space from the PMM rounded up to the nearest power of two
    // The minimum size will be 0.1% of total free space
    const vmm_size = std.math.ceilPowerOfTwoAssert(usize, pmm.get_free_size() * 4 / 1000);
    const last: *limine.limine_memmap_entry = MMAP_REQUEST.response.*.entries[MMAP_REQUEST.response.*.entry_count - 1];
    kvmm = VMM.create(virt(0) + last.base + last.length, vmm_size, 0b11 | (1 << 63), &kmapper);
    return .success;
}

fn kheap_init() kernel.Task.Ret {
    // The kernel heap size will be a percentage of the total free space from the PMM
    // For now it will be 0.1%
    // It must be less than or equal to the VMM size
    const kheap_size = (pmm.get_free_size() * 2 / 1000) / 0x1000 * 0x1000;
    kheap = FixedBufferAllocator.init(kvmm.allocator().alloc(u8, kheap_size) catch return .failed);
    return .success;
}

/// Converts virtual address to physical address
/// DOES NOT CHECK FOR OVERFLOW
pub inline fn phys(addr: usize) usize {
    return addr - HHDM_REQUEST.response.*.offset;
}

/// Converts physical address to virtual address
/// DOES NOT CHECK FOR OVERFLOW
pub inline fn virt(addr: usize) usize {
    return addr + HHDM_REQUEST.response.*.offset;
}
