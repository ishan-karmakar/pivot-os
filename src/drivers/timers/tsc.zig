const kernel = @import("kernel");
const timers = kernel.drivers.timers;
const hpet = kernel.drivers.timers.hpet;
const pit = kernel.drivers.timers.pit;
const cpu = kernel.drivers.cpu;
const std = @import("std");
const log = @import("std").log.scoped(.tsc);

pub var vtable: timers.VTable = .{
    .min_interval = 0,
    .max_interval = 0,
    .init = init,
    .sleep = sleep,
    .time = time,
};

const CALIBRATION_NS = 5_000_000; // 5 ms
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
    _ = cal_timer.sleep(CALIBRATION_NS);
    const after = rdtsc();
    vtable.min_interval = CALIBRATION_NS / @as(f64, @floatFromInt(after - before));
    update_max_interval();
    log.info("Initialized TSC", .{});
    initialized = true;
    return true;
}

fn update_max_interval() void {
    vtable.max_interval = std.math.maxInt(usize) - rdtsc();
    if (vtable.min_interval < 1) {
        const tmp: f64 = @floatFromInt(vtable.max_interval);
        vtable.max_interval = @intFromFloat(tmp * vtable.min_interval);
    }
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
    return 0;
}

fn sleep(ns: usize) bool {
    _ = ns;
    // update_max_interval();
    // if (ns > vtable.max_interval) return false;

    // const ticks = ns / vtable.min_interval;
    // const end = rdtsc() + ticks;
    // while (rdtsc() < end) asm volatile ("pause");
    return true;
}
