const kernel = @import("kernel");
const timers = kernel.drivers.timers;
const hpet = kernel.drivers.timers.hpet;
const pit = kernel.drivers.timers.pit;
const cpu = kernel.drivers.cpu;
const std = @import("std");
const log = @import("std").log.scoped(.tsc);

pub var vtable: timers.GTSVTable = .{
    .time = time,
    .deinit = deinit,
};

const CALIBRATION_NS = 1_000_000; // 5 ms

pub var Task = kernel.Task{
    .name = "TSC",
    .init = init,
    // We don't depend on any timers here because TSC task runs after other timers in timers/index.zig. Probably not the most clear but it works :/
    .dependencies = &.{},
};

var ticks_per_ns: f64 = undefined;

fn init() bool {
    const freqs = cpu.cpuid(0x15, 0);
    if (freqs.ebx != 0 and freqs.ecx != 0) {
        @panic("Handle frequency calculation from CPUID");
    }
    const cpuid = cpu.cpuid(0x80000007, 0);
    if (cpuid.edx & (1 << 8) == 0) {
        log.debug("Invariant TSC not supported", .{});
        return false;
    }
    const before = rdtsc();
    _ = timers.sleep(CALIBRATION_NS);
    const after = rdtsc();
    ticks_per_ns = @floatFromInt(after - before);
    ticks_per_ns /= CALIBRATION_NS;
    timers.set_gts(&vtable);
    return true;
}

fn deinit() void {}

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
    // Just multiplying by arbitary number (1000) to hopefully move the decimals into the integer part before converting
    return rdtsc() / @as(usize, @intFromFloat(ticks_per_ns * 1_000)) * 1_000;
}
