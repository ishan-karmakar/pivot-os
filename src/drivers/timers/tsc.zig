const kernel = @import("kernel");
const timers = kernel.drivers.timers;
const hpet = kernel.drivers.timers.hpet;
const pit = kernel.drivers.timers.pit;
const cpu = kernel.drivers.cpu;
const log = @import("std").log.scoped(.tsc);

pub const vtable: timers.VTable = .{
    .init = init,
    .sleep = sleep,
    .time = time,
};

var ticks_per_ns: f64 = undefined;
const CALIBRATION_NS = 10_000_000; // 10 ms
var initialized: ?bool = null;

fn init() bool {
    defer initialized = initialized orelse false;
    if (initialized) |i| return i;
    const freqs = cpu.cpuid(0x15, 0);
    if (freqs.ebx != 0 and freqs.ecx != 0) {
        @panic("Handle frequency calculation from CPUID");
    }
    var cal_timer: *const timers.VTable = undefined;
    if (hpet.vtable.init()) {
        cal_timer = &hpet.vtable;
    } else if (pit.vtable.init()) {
        cal_timer = &pit.vtable;
    } else return false;
    const cpuid = cpu.cpuid(0x80000007, 0);
    if (cpuid.edx & (1 << 8) == 0) {
        log.debug("Invariant TSC not supported", .{});
        return false;
    }
    const before = rdtsc();
    cal_timer.sleep(CALIBRATION_NS); // 10 ms
    const after = rdtsc();
    ticks_per_ns = @as(f64, @floatFromInt(after - before)) / CALIBRATION_NS;
    log.info("Initialized TSC", .{});
    initialized = true;
    return true;
}

fn rdtsc() usize {
    var upper: u32 = undefined;
    var lower: u32 = undefined;
    asm volatile ("rdtsc"
        : [edx] "={edx}" (upper),
          [eax] "={eax}" (lower),
    );
    return (@as(u64, @intCast(upper)) << 32) | lower;
}

fn time() usize {
    return @intFromFloat(@as(f64, @floatFromInt(rdtsc())) / ticks_per_ns);
}

fn sleep(ns: usize) void {
    const start = time();
    while (time() < start + ns) asm volatile ("pause");
}
