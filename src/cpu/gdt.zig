const kernel = @import("root");
const smp = kernel.cpu.smp;
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

var static_initialized = false;
var dynamic_initialized = false;

pub fn init_static() void {
    if (static_initialized)
        return kernel.lib.logger.already_initialized(log, "Static GDT");
    gdtr.addr = @intFromPtr(&static_gdt);
    lgdt();
    static_initialized = true;
    kernel.lib.logger.successfully_initialized(log, "Static GDT");
}

pub fn init_dynamic() !void {
    if (dynamic_initialized)
        return kernel.lib.logger.already_initialized(log, "Dynamic GDT");
    mem.init_kheap() catch |err|
        return kernel.lib.logger.failed_initialization(log, "Dynamic GDT", err);
    if (smp.SMP_REQUEST.response == null)
        return kernel.lib.logger.failed_initialization(log, "Dynamic GDT", error.SMPUnavailable);

    const buf = mem.kheap.allocator().alloc(Entry, 5 + smp.SMP_REQUEST.response.*.cpu_count * 2) catch |err|
        return kernel.lib.logger.failed_initialization(log, "Dynamic GDT", err);
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
    dynamic_initialized = true;
    kernel.lib.logger.successfully_initialized(log, "Dynamic GDT");
}

pub fn init_ap() void {
    mem.init_kmapper_ap();
    lgdt();
    kernel.lib.logger.successfully_initialized(log, "GDT");
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
