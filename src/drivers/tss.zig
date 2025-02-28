const kernel = @import("kernel");
const std = @import("std");
const mem = kernel.lib.mem;
const smp = kernel.lib.smp;
const gdt = kernel.drivers.gdt;

// align(1) is kind of a hack to enforce no padding but have arrays
pub const TSS = extern struct {
    rsv0: u32,
    rsp: [3]u64 align(1),
    rsv1: u64 align(1),
    ist: [7]u64 align(1),
    rsv2: u64 align(1),
    rsv3: u16 align(1),
    iopb: u16 align(1),
};

pub var Task = kernel.Task{
    .name = "TSS",
    .init = init,
    .dependencies = &.{
        .{ .task = &smp.Task },
        .{ .task = &gdt.DynamicTask },
    },
};

pub var TaskAP = kernel.Task{
    .name = "TSS (AP)",
    .init = init,
    .dependencies = &.{
        .{ .task = &gdt.DynamicTaskAP },
    },
};

fn init() kernel.Task.Ret {
    const tss = mem.kheap.allocator().create(TSS) catch return .failed;
    tss.iopb = @sizeOf(TSS);
    smp.cpu_info(null).tss = tss;

    // // WARNING: Most of TSS is left in an undefined state
    // const tbl: [*]gdt.Entry = @ptrFromInt(gdt.gdtr.addr);
    return .success;
}
