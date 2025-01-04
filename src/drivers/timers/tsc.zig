const kernel = @import("kernel");
const timers = kernel.drivers.timers;
const hpet = kernel.drivers.timers.hpet;
const pit = kernel.drivers.timers.pit;
const cpu = kernel.drivers.cpu;
const std = @import("std");
const log = @import("std").log.scoped(.tsc);

pub var vtable: timers.VTable = .{
    .time = time,
    .callback = null,
};

const CALIBRATION_NS = 1_000_000; // 5 ms

pub var Task = kernel.Task{
    .name = "TSC",
    .init = init,
    .dependencies = &.{
        .{ .task = &timers.TimerTask },
    },
};

var ticks_per_ns: f64 = undefined;

fn init() kernel.Task.Ret {
    if (timers.gts != null) return .skipped;
    const freqs = cpu.cpuid(0x15, 0);
    if (freqs.ebx != 0 and freqs.ecx != 0) {
        @panic("Handle frequency calculation from CPUID");
    }
    const cpuid = cpu.cpuid(0x80000007, 0);
    if (cpuid.edx & (1 << 8) == 0) {
        log.debug("Invariant TSC not supported", .{});
        return .failed;
    }
    const before = rdtsc();
    _ = timers.sleep(CALIBRATION_NS);
    const after = rdtsc();
    ticks_per_ns = @floatFromInt(after - before);
    ticks_per_ns /= CALIBRATION_NS;
    timers.gts = &vtable;
    return .success;
}

fn deinit() void {}

fn rdtsc() usize {
    // TODO: rdtscp? mfence + lfence? https://stackoverflow.com/questions/27693145/rdtscp-versus-rdtsc-cpuid
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
