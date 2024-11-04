const log = @import("std").log.scoped(.lapic);
const cpu = @import("kernel").drivers.cpu;
const mem = @import("kernel").lib.mem;
const idt = @import("kernel").drivers.idt;

const MSR = 0x1B;
const SPURIOUS_VEC = 0xFF;
var addr: ?usize = null;

const SPURIOUS_OFF = 0xF0;

pub fn bsp_init() void {
    const msr = cpu.rdmsr(MSR);
    if (msr & (1 << 11) == 0) @panic("APIC is not enabled");

    const cpuid = cpu.cpuid(1, 0);
    if (cpuid.edx & (1 << 9) > 0) {
        addr = mem.virt(msr & 0xffffffffff000);
    } else if (cpuid.ecx & (1 << 21) == 0) @panic("Neither X2APIC nor XAPIC is set in CPUID");

    idt.set_ent(SPURIOUS_VEC, idt.create_irq(false, "spurious_handler"));
    write_reg(SPURIOUS_OFF, (@as(u32, 1) << 8) | SPURIOUS_VEC);
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

export fn spurious_handler() void {}
