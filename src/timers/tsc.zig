const kernel = @import("root");
const timers = kernel.timers;
const limine = @import("limine");
const cpu = kernel.cpu;
const std = @import("std");
const log = @import("std").log.scoped(.tsc);

export var TSC_FREQ_REQUEST = limine.limine_tsc_frequency_request{
    .id = kernel.LIMINE_REQUEST_ID(0x10f2ee1d87d195e4, 0xf747a2b78f6ddb31),
};

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
    if (TSC_FREQ_REQUEST.response) |resp| {
        log.debug("TSC Frequency (from Limine) is {} Hz", .{resp.*.frequency});
        period = @as(f64, 1_000_000_000) / @as(f64, @floatFromInt(resp.*.frequency));
    } else {
        log.debug("TSC Frequency response is empty, falling back to manual calibration", .{});
        const before = rdtsc();
        timers.sleep(CALIBRATION_NS);
        const after = rdtsc();
        period = @as(f64, @floatFromInt(after - before)) / @as(f64, CALIBRATION_NS);
        log.debug("TSC frequency (manually calibrated) is {} Hz", .{1_000_000_000 / period});
    }
    initialized = true;
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
