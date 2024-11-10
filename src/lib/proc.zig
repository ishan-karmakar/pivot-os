const mem = @import("kernel").lib.mem;
const FixedBufferAllocator = @import("std").heap.FixedBufferAllocator;

// If these are null, then we use kheap instead
// mapper is not a pointer because we actually access mapper in kernel address space
heap: ?*FixedBufferAllocator,
mapper: ?mem.Mapper,
vmm: ?*mem.VMM,

pub fn create(func: fn () void) void {
    const alloc = mem.kheap.allocator();
    const mapper = mem.Mapper.create(mem.virt(mem.pmm.frame()));
    const vmm = alloc.create(mem.VMM) catch @panic("OOM");
}
