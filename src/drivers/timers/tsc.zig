const kernel = @import("kernel");
const VTable = kernel.drivers.timers.VTable;
const hpet = kernel.drivers.timers.hpet;
const cpu = kernel.drivers.cpu;
const log = @import("std").log.scoped(.tsc);

pub const vtable: VTable = .{
    .init = init,
    .sleep = sleep,
    .time = time,
};

var ticks_per_ns: f64 = undefined;
const CALIBRATION_NS = 10_000_000; // 10 ms

fn init() bool {
    var cal_timer: *const VTable = undefined;
    if (hpet.vtable.init()) {
        cal_timer = &hpet.vtable;
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
