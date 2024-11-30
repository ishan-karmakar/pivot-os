const kernel = @import("kernel");
const mem = kernel.lib.mem;
const cpu = kernel.drivers.cpu;
const FixedBufferAllocator = @import("std").heap.FixedBufferAllocator;
const Allocator = @import("std").mem.Allocator;

const PROC_STACK_SIZE = 0x1000;
const PROC_HEAP_SIZE = 0x1000;

const ProcStatus = enum {
    ready,
    sleeping,
    dead,
};

// If these are null, then we use kheap instead
// mapper is not a pointer because we actually access mapper in kernel address space
mapper: mem.Mapper,
vmm: ?mem.VMM,
ef: cpu.Status,
status: ProcStatus = .ready,
priority: u8,
stack: ?[]u8,
next: ?*@This() = null,

// pub fn create(func: fn () void) @This() {
//     const pml4 = mem.virt(mem.pmm.frame());
//     for (@as(mem.Mapper.Table, @ptrFromInt(pml4))[0..256]) |*c| c.* = 0;
//     for (@as(mem.Mapper.Table, @ptrFromInt(pml4))[256..], mem.kmapper.pml4[256..]) |*c, p| {
//         c.* = p;
//     }
//     const mapper = mem.kheap.allocator().create(mem.Mapper) catch @panic("OOM");
//     mapper.* = mem.Mapper.create(pml4);
//     cpu.set_cr3(mem.phys(pml4));
//     const vmm = mem.VMM.create(0x1000, 0x4000, (1 << 63) | 0b10, mapper);
//     const heap = FixedBufferAllocator.init(vmm.allocator().alloc(u8, PROC_HEAP_SIZE) catch @panic("OOM"));
//     const stack = vmm.allocator().alloc(u8, PROC_STACK_SIZE) catch @panic("OOM");
//     cpu.set_cr3(mem.phys(@intFromPtr(mem.kmapper.pml4)));

//     return .{
//         .heap = heap.allocator(), // not threadSafeAllocator because theoretically this should be the only process using it
//         .mapper = mapper,
//         .vmm = vmm,
//         .ef = ef,
//     };
// }
