const kernel = @import("kernel");
const log = @import("std").log.scoped(.lapic);
const cpu = kernel.drivers.cpu;

const MSR = 0x1B;
const SPURIOUS_VEC = 0xFF;
var addr: ?usize = null;

const TPR_OFF = 0x80;
const EOI_OFF = 0xB0;
const SPURIOUS_OFF = 0xF0;
const LVT_OFF = 0x320;
const INITIAL_COUNT_OFF = 0x380;
const CONFIG_OFF = 0x3E0;
const CUR_COUNT_OFF = 0x390;
const ICRLO = 0x300;
const ICRHI = 0x310;

pub fn bsp_init() void {
    var msr = cpu.rdmsr(MSR) | (1 << 11);

    const cpuid = cpu.cpuid(1, 0);
    if (cpuid.ecx & (1 << 21) > 0) {
        msr |= 1 << 10;
    } else if (cpuid.edx & (1 << 9) > 0) {
        addr = msr & ~@as(u64, 0xFFF);
        kernel.lib.mem.kmapper.map(addr.?, addr.?, (1 << 63) | 0b10);
    } else @panic("Neither x2APIC nor xAPIC is set in CPUID");
    cpu.wrmsr(MSR, msr);
    write_reg(TPR_OFF, 0);

    kernel.drivers.idt.vec2handler(SPURIOUS_VEC).reserved = true;
    write_reg(SPURIOUS_OFF, (@as(u32, 1) << 8) | SPURIOUS_VEC);
    log.info("Initialized {s}", .{if (addr == null) "x2APIC" else "xAPIC"});
}

pub inline fn eoi() void {
    write_reg(EOI_OFF, 0);
}

pub fn write_reg(off: u32, val: u64) void {
    if (addr) |a| {
        @as(*u32, @ptrFromInt(a + off)).* = @truncate(val);
    } else cpu.wrmsr((off >> 4) + 0x800, val);
}

pub fn read_reg(off: u32) u64 {
    if (addr) |a| {
        return @intCast(@as(*const u32, @ptrFromInt(a + off)).*);
    } else return cpu.rdmsr((off >> 4) + 0x800);
}
