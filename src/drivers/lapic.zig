const log = @import("std").log.scoped(.lapic);
const cpu = @import("kernel").drivers.cpu;
const mem = @import("kernel").lib.mem;
const idt = @import("kernel").drivers.idt;
const scheduler = @import("kernel").lib.scheduler;

const MSR = 0x1B;
const SPURIOUS_VEC = 0xFF;
var addr: ?usize = null;

const SPURIOUS_OFF = 0xF0;
const LVT_OFF = 0x320;
const EOI_OFF = 0xB0;
pub const INITIAL_COUNT_OFF = 0x380;
pub const CONFIG_OFF = 0x3E0;
pub const CUR_COUNT_OFF = 0x390;

pub fn bsp_init() void {
    var msr = cpu.rdmsr(MSR) | (1 << 11);

    const cpuid = cpu.cpuid(1, 0);
    if (cpuid.edx & (1 << 9) > 0) {
        addr = msr & ~@as(u64, 0xFFF);
        mem.kmapper.map(addr.?, addr.?, (1 << 63) | 0b10);
    } else if (cpuid.ecx & (1 << 21) == 0) {
        msr |= (1 << 10);
    } else @panic("Neither x2APIC nor xAPIC is set in CPUID");
    log.debug("Using {s}", .{if (addr == null) "x2APIC" else "xAPIC"});
    cpu.wrmsr(MSR, msr);

    idt.set_ent(SPURIOUS_VEC, idt.create_irq(0, "spurious_handler"));
    write_reg(SPURIOUS_OFF, (@as(u32, 1) << 8) | SPURIOUS_VEC);
    write_reg(LVT_OFF, scheduler.SCHED_VEC);
    write_reg(CONFIG_OFF, 1);
    log.info("Initialized Local APIC", .{});
}

pub fn ap_init() void {
    var msr = cpu.rdmsr(MSR) | (1 << 11);
    if (addr == null) msr |= (1 << 10);
    cpu.wrmsr(MSR, msr);
    write_reg(SPURIOUS_OFF, (1 << 8) | SPURIOUS_VEC);
    write_reg(LVT_OFF, scheduler.SCHED_VEC);
    write_reg(CONFIG_OFF, 1);
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

export fn spurious_handler(status: *const cpu.Status, _: usize) *const cpu.Status {
    return status;
}
