const kernel = @import("kernel");
const log = @import("std").log.scoped(.lapic);
const cpu = kernel.drivers.cpu;

const MSR = 0x1B;
const SPURIOUS_VEC = 0xFF;

const TPR_OFF = 0x80;
const EOI_OFF = 0xB0;
const SPURIOUS_OFF = 0xF0;
const LVT_OFF = 0x320;
const ICRLO = 0x300;
const ICRHI = 0x310;

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

pub fn bsp_init() kernel.Task.Ret {
    var msr = cpu.rdmsr(MSR) | (1 << 11);

    const cpuid = cpu.cpuid(1, 0);
    if (cpuid.ecx & (1 << 21) > 0) {
        msr |= 1 << 10;
        read_reg = x2apic_read_reg;
        write_reg = x2apic_write_reg;
        log.debug("Found x2APIC", .{});
    } else if (cpuid.edx & (1 << 9) > 0) {
        addr = msr & ~@as(u64, 0xFFF);
        kernel.lib.mem.kmapper.map(addr, addr, (1 << 63) | 0b10);
        read_reg = xapic_read_reg;
        write_reg = xapic_write_reg;
        log.debug("Found xAPIC", .{});
    } else return .failed;
    cpu.wrmsr(MSR, msr);
    write_reg(TPR_OFF, 0);

    kernel.drivers.idt.vec2handler(SPURIOUS_VEC).reserved = true;
    write_reg(SPURIOUS_OFF, (@as(u32, 1) << 8) | SPURIOUS_VEC);
    return .success;
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
