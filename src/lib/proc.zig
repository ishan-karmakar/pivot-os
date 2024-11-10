const mem = @import("kernel").lib.mem;
const cpu = @import("kernel").drivers.cpu;
const FixedBufferAllocator = @import("std").heap.FixedBufferAllocator;
const Allocator = @import("std").mem.Allocator;

const PROC_STACK_SIZE = 0x1000;
const PROC_HEAP_SIZE = 0x1000;

// If these are null, then we use kheap instead
// mapper is not a pointer because we actually access mapper in kernel address space
heap: ?Allocator,
mapper: ?*mem.Mapper,
vmm: ?*mem.VMM,
ef: cpu.Status,
new: bool = true,

pub fn create(func: fn () void) @This() {
    const alloc = mem.kheap.allocator();
    const pml4 = mem.virt(mem.pmm.frame());
    for (@as(mem.Mapper.Table, @ptrFromInt(pml4))[0..256]) |*c| c.* = 0;
    for (@as(mem.Mapper.Table, @ptrFromInt(pml4))[256..], mem.kmapper.pml4[256..]) |*c, p| {
        c.* = p;
    }
    const mapper = alloc.create(mem.Mapper) catch @panic("OOM");
    mapper.* = mem.Mapper.create(pml4);
    const vmm = alloc.create(mem.VMM) catch @panic("OOM");
    const heap = alloc.create(FixedBufferAllocator) catch @panic("OOM");
    cpu.set_cr3(mem.phys(pml4));
    vmm.* = mem.VMM.create(0x1000, 0x4000, (1 << 63) | 0b10, mapper);
    heap.* = FixedBufferAllocator.init(vmm.allocator().alloc(u8, PROC_HEAP_SIZE) catch @panic("OOM"));
    const stack = vmm.allocator().alloc(u8, PROC_STACK_SIZE) catch @panic("OOM");
    cpu.set_cr3(mem.phys(@intFromPtr(mem.kmapper.pml4)));
    const ef = create_ef(func);
    ef.iret_status.rsp = @intFromPtr(stack.ptr) + PROC_STACK_SIZE;

    return .{
        .heap = heap.allocator(), // not threadSafeAllocator because theoretically this should be the only process using it
        .mapper = mapper,
        .vmm = vmm,
        .ef = ef,
    };
}

pub inline fn create_ef(func: fn () void) cpu.Status {
    return .{
        .r15 = 0,
        .r14 = 0,
        .r13 = 0,
        .r12 = 0,
        .r11 = 0,
        .r10 = 0,
        .r9 = 0,
        .r8 = 0,
        .rdi = 0,
        .rsi = 0,
        .rbp = 0,
        .rdx = 0,
        .rcx = 0,
        .rbx = 0,
        .rax = 0,
        .iret_status = .{
            .rip = @intFromPtr(&func),
            .cs = 0x8,
            .rflags = 0x202,
            .rsp = 0,
            .ss = 0x10,
        },
    };
}
