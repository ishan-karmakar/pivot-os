const kernel = @import("root");
const timers = kernel.timers;
const hpet = kernel.timers.hpet;
const pit = kernel.timers.pit;
const cpu = kernel.drivers.cpu;
const std = @import("std");
const log = @import("std").log.scoped(.tsc);

pub const VTable = timers.VTable{
    .time = time,
    .capabilities = .{
        .FIXED = false,
    },
    .callback = null,
};

const CALIBRATION_TIME = 1_000_000;

pub var Task = kernel.Task{
    .name = "TSC",
    .init = init,
    .dependencies = &.{},
};

// Period between ticks in nanoseconds
pub var period: f64 = undefined;

fn init() kernel.Task.Ret {
    const cpuid = cpu.cpuid(0x80000007, 0);
    if (cpuid.edx & (1 << 8) == 0) {
        log.debug("Invariant TSC not supported", .{});
        return .failed;
    }
    const freqs = cpu.cpuid(0x15, 0);
    const freqs2 = cpu.cpuid(0x16, 0);
    if (freqs.eax != 0 and freqs.ebx != 0) {
        if (freqs.ecx != 0) {
            period = @as(f64, @floatFromInt(@as(u64, freqs.ecx) * freqs.ebx)) / @as(f64, @floatFromInt(freqs.eax)) / 1_000_000_000.0;
            return .success;
        } else if (freqs2.eax != 0) {
            period = @as(f64, @floatFromInt(@as(u64, freqs2.eax) * 10_000_000 * freqs.eax)) / @as(f64, @floatFromInt(freqs.ebx)) / 1_000_000_000.0;
            return .success;
        }
    }
    const before = rdtsc();
    timers.sleep(CALIBRATION_TIME);
    const after = rdtsc();
    period = @as(f64, @floatFromInt(after - before)) / @as(f64, CALIBRATION_TIME);
    return .success;
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
