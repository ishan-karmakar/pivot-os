const kernel = @import("root");
const timers = kernel.drivers.timers;
const hpet = kernel.drivers.timers.hpet;
const pit = kernel.drivers.timers.pit;
const cpu = kernel.drivers.cpu;
const std = @import("std");
const log = @import("std").log.scoped(.tsc);

const CLOCKSOURCE = timers.ClockSource{
    .rating = 300,
    .read = time,
};

const CALIBRATION_NS = 1_000_000; // 1 ms

// Period between ticks in nanoseconds
pub var period: f64 = undefined;
var initialized = false;

pub fn init() !void {
    if (initialized)
        return kernel.lib.logger.already_initialized(log, "TSC");

    if (!is_supported())
        return kernel.lib.logger.failed_initialization(log, "TSC", error.InvariantTSCUnsupported);
    const freqs = cpu.cpuid(0x15, 0);
    const freqs2 = cpu.cpuid(0x16, 0);
    if (freqs.eax != 0 and freqs.ebx != 0) {
        if (freqs.ecx != 0) {
            period = @as(f64, @floatFromInt(@as(u64, freqs.ecx) * freqs.ebx)) / @as(f64, @floatFromInt(freqs.eax)) / 1_000_000_000.0;
            initialized = true;
        } else if (freqs2.eax != 0) {
            period = @as(f64, @floatFromInt(@as(u64, freqs2.eax) * 10_000_000 * freqs.eax)) / @as(f64, @floatFromInt(freqs.ebx)) / 1_000_000_000.0;
            initialized = true;
        }
    }
    if (!initialized) {
        const before = rdtsc();
        timers.sleep(CALIBRATION_NS) catch |err|
            return kernel.lib.logger.failed_initialization(log, "TSC", err);
        const after = rdtsc();
        period = @as(f64, @floatFromInt(after - before)) / @as(f64, CALIBRATION_NS);
        initialized = true;
    }
    timers.register_clocksource(&CLOCKSOURCE);
    kernel.lib.logger.successfully_initialized(log, "TSC");
}

pub fn is_supported() bool {
    const cpuid = cpu.cpuid(0x80000007, 0);
    return cpuid.edx & (1 << 8) != 0;
}

pub fn rdtsc() usize {
    var upper: u32 = 0;
    var lower: u32 = 0;
    asm volatile ("rdtsc"
        : [edx] "={edx}" (upper),
          [eax] "={eax}" (lower),
    );
    return (@as(u64, @intCast(upper)) << 32) | lower;
}

fn time() usize {
    return @intFromFloat(@as(f128, @floatFromInt(rdtsc())) * period);
}

pub fn ns2ticks(ns: usize) usize {
    return @intFromFloat(@as(f128, @floatFromInt(ns)) / period);
}
