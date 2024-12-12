const kernel = @import("kernel");
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

pub fn initEarly() void {
    gdtr.addr = @intFromPtr(&static_gdt);
    lgdt();
    log.info("Loaded static GDT", .{});
}

// initStage2

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
