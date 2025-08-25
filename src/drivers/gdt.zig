const kernel = @import("root");
const smp = kernel.lib.smp;
const mem = kernel.lib.mem;
const log = @import("std").log.scoped(.gdt);

const Entry = packed struct {
    limit0: u16 = 0xFFFF,
    base0: u16 = 0,
    base1: u8 = 0,
    access: u8,
    limit1: u4 = 0xF,
    flags: u4,
    base2: u8 = 0,
};

const GDTR = packed struct {
    size: u16,
    addr: usize,
};

var static_gdt: [3]Entry = .{
    .{
        .access = 0,
        .flags = 0,
    },
    .{
        .access = 0b10011011,
        .flags = 0b10,
    },
    .{
        .access = 0b10010011,
        .flags = 0,
    },
};

var gdtr = GDTR{
    .addr = 0,
    .size = static_gdt.len * @sizeOf(Entry) - 1,
};

pub var StaticTask = kernel.Task{
    .name = "Static GDT",
    .init = init_static,
    .dependencies = &.{},
};

pub var DynamicTask = kernel.Task{
    .name = "Dynamic GDT",
    .init = init_dynamic,
    .dependencies = &.{
        .{ .task = &mem.KHeapTask },
    },
};

pub var DynamicTaskAP = kernel.Task{
    .name = "Dynamic GDT (AP)",
    .init = init_dynamic_ap,
    .dependencies = &.{
        .{ .task = &mem.KMapperTaskAP },
    },
};

fn init_static() kernel.Task.Ret {
    gdtr.addr = @intFromPtr(&static_gdt);
    lgdt();
    return .success;
}

fn init_dynamic() kernel.Task.Ret {
    if (smp.SMP_REQUEST.response == null) return .failed;
    const buf = mem.kheap.allocator().alloc(Entry, 5 + smp.SMP_REQUEST.response.?.cpu_count * 2) catch return .failed;
    for (0..3) |i| buf[i] = static_gdt[i];
    buf[3] = .{
        .access = 0b11111011,
        .flags = 0b10,
    };
    buf[4] = .{
        .access = 0b11110011,
        .flags = 0,
    };
    gdtr.size = @intCast(buf.len * @sizeOf(Entry) - 1);
    gdtr.addr = @intFromPtr(buf.ptr);
    lgdt();
    return .success;
}

fn init_dynamic_ap() kernel.Task.Ret {
    lgdt();
    return .success;
}

pub fn lgdt() void {
    asm volatile ("lgdt (%[gdtr])"
        :
        : [gdtr] "r" (&gdtr),
    );

    asm volatile (
        \\mov %[data], %%ds
        \\mov %[data], %%es
        \\mov %[data], %%fs
        \\mov %[data], %%gs
        \\mov %[data], %%ss
        :
        : [data] "r" (@as(u16, 0x10)),
    );

    asm volatile (
        \\push %[code]
        \\push $1f
        \\lretq
        \\1:
        :
        : [code] "i" (0x8),
    );
}
