const kernel = @import("root");
const log = @import("std").log.scoped(.lapic);
const cpu = kernel.drivers.cpu;

const MSR = 0x1B;
const SPURIOUS_VEC = 0xFF;

const TPR_OFF = 0x80;
const EOI_OFF = 0xB0;
const SPURIOUS_OFF = 0xF0;
const CONFIG_OFF = 0x3E0;
const ICRLO = 0x300;
const ICRHI = 0x310;

// 0b0 - Divide by 2
// 0b1 - Divide by 4
// 0b10 - Divide by 8
// 0b11 - Divide by 16
// 0b1000 - Divide by 32
// 0b1001 - Divide by 64
// 0b1010 - Divide by 128
// 0b1011 - Divide by 1
const TDIV = 1;

var addr: usize = undefined;
pub var read_reg: *const fn (off: u32) u64 = undefined;
pub var write_reg: *const fn (off: u32, val: u64) void = undefined;

pub var Task = kernel.Task{
    .name = "Local APIC",
    .init = bsp_init,
    .dependencies = &.{
        .{ .task = &kernel.drivers.idt.Task },
        .{ .task = &kernel.lib.mem.KMapperTask },
    },
};

pub var TaskAP = kernel.Task{
    .name = "Local APIC (AP)",
    .init = ap_init,
    .dependencies = &.{
        .{ .task = &kernel.lib.mem.KMapperTaskAP },
    },
};

fn bsp_init() kernel.Task.Ret {
    var msr = cpu.rdmsr(MSR) | (1 << 11);
    const cpuid = cpu.cpuid(1, 0);
    if (cpuid.ecx & (1 << 21) > 0) {
        msr |= 1 << 10;
        read_reg = x2apic_read_reg;
        write_reg = x2apic_write_reg;
        log.debug("Found x2APIC", .{});
    } else if (cpuid.edx & (1 << 9) > 0) {
        addr = msr & ~@as(u64, 0xFFF);
        kernel.lib.mem.kmapper.map(addr, addr, (1 << 63) | 0b11);
        read_reg = xapic_read_reg;
        write_reg = xapic_write_reg;
        log.debug("Found xAPIC", .{});
    } else return .failed;
    cpu.wrmsr(MSR, msr);

    kernel.drivers.idt.vec2handler(SPURIOUS_VEC).reserved = true;
    write_reg(SPURIOUS_OFF, (@as(u32, 1) << 8) | SPURIOUS_VEC);
    write_reg(TPR_OFF, 0);
    write_reg(CONFIG_OFF, TDIV);
    return .success;
}

fn ap_init() kernel.Task.Ret {
    var msr = cpu.rdmsr(MSR) | (1 << 11);
    if (read_reg == x2apic_read_reg) msr |= 1 << 10;
    cpu.wrmsr(MSR, msr);

    write_reg(SPURIOUS_OFF, (@as(u32, 1) << 8) | SPURIOUS_VEC);
    write_reg(TPR_OFF, 0);
    write_reg(CONFIG_OFF, TDIV);
    return .success;
}

pub fn ipi(dest: u8, vec: u8) void {
    if (read_reg == x2apic_read_reg) {
        write_reg(ICRLO, (@as(u64, dest) << 56) | vec);
    } else {
        write_reg(ICRHI, @as(u32, dest) << 24);
        write_reg(ICRLO, vec);
    }
}

pub inline fn eoi() void {
    write_reg(EOI_OFF, 0);
}

fn xapic_write_reg(off: u32, val: u64) void {
    @as(*u32, @ptrFromInt(addr + off)).* = @truncate(val);
}

fn x2apic_write_reg(off: u32, val: u64) void {
    cpu.wrmsr((off >> 4) + 0x800, val);
}

fn xapic_read_reg(off: u32) u64 {
    return @intCast(@as(*const u32, @ptrFromInt(addr + off)).*);
}

fn x2apic_read_reg(off: u32) u64 {
    return cpu.rdmsr((off >> 4) + 0x800);
}
