const kernel = @import("root");
pub const pmm = @import("pmm.zig");
pub const Mapper = @import("mapper.zig");
pub const VMM = @import("vmm.zig");
pub const SLUB = @import("SLUB.zig");
const limine = @import("limine");
const std = @import("std");
const log = std.log.scoped(.mem);

pub var kmapper: Mapper = undefined;
pub var kvmm: VMM = undefined;
pub var kheap: SLUB = .{};

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

var kmapper_initialized = false;
var kvmm_initialized = false;

pub fn init_kmapper() !void {
    if (kmapper_initialized)
        return kernel.lib.logger.already_initialized(log, "Kernel mapper");
    pmm.init() catch |err| {
        log.err("Failed to initialize kernel mapper because PMM failed: {}", .{err});
        return err;
    };
    if (PAGING_REQUEST.response == null) {
        log.err("Failed to initialize kernel mapper because Limine paging request is empty", .{});
        return error.PagingUnavailable;
    } else if (PAGING_REQUEST.response.*.mode != PAGING_REQUEST.mode) {
        log.err("Failed to initialize kernel mapper because Limine paging mode is not our expected mode", .{});
        return error.PagingModeUnsupported;
    }
    kmapper = Mapper.create(virt(asm volatile ("mov %%cr3, %[result]"
        : [result] "=r" (-> usize),
    ) & 0xfffffffffffffffe));
    kmapper_initialized = true;
    kernel.lib.logger.successfully_initialized(log, "Kernel mapper");
}

pub fn init_kmapper_ap() void {
    kernel.cpu.set_cr3(phys(@intFromPtr(kmapper.pml4)));
    return kernel.lib.logger.successfully_initialized(log, "Kernel mapper");
}

pub fn init_kvmm() !void {
    if (kvmm_initialized)
        return kernel.lib.logger.already_initialized(log, "Kernel VMM");
    init_kmapper() catch |err|
        return kernel.lib.logger.failed_initialization(log, "Kernel VMM", err);
    // The VMM size will be a percentage of the total free space from the PMM rounded up to the nearest power of two
    // The minimum size will be 0.1% of total free space
    const vmm_size = std.math.ceilPowerOfTwoAssert(usize, pmm.get_free_size() * 2 / 100);
    const last: *limine.limine_memmap_entry = MMAP_REQUEST.response.*.entries[MMAP_REQUEST.response.*.entry_count - 1];
    kvmm = VMM.create(virt(0) + last.base + last.length, vmm_size, 0b11 | (1 << 63), &kmapper);
    kvmm_initialized = true;
    kernel.lib.logger.successfully_initialized(log, "Kernel VMM");
}

pub fn init_kheap() !void {
    init_kvmm() catch |err|
        return kernel.lib.logger.failed_initialization(log, "Kernel heap", err);
    kernel.lib.logger.successfully_initialized(log, "Kernel heap");
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
