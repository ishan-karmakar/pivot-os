const kernel = @import("kernel");
const timers = kernel.drivers.timers;
const hpet = kernel.drivers.timers.hpet;
const pit = kernel.drivers.timers.pit;
const cpu = kernel.drivers.cpu;
const std = @import("std");
const log = @import("std").log.scoped(.tsc);

pub var GTSVTable = timers.GTSVTable{
    .requires_calibration = true,
    .time = time,
};

const CALIBRATION_NS = 1_000_000; // 1 ms

pub var GTSTask = kernel.Task{
    .name = "TSC",
    .init = init,
    .dependencies = &.{},
};

pub var hertz: usize = undefined;

fn init() kernel.Task.Ret {
    const cpuid = cpu.cpuid(0x80000007, 0);
    if (cpuid.edx & (1 << 8) == 0) {
        log.debug("Invariant TSC not supported", .{});
        return .failed;
    }
    const freqs = cpu.cpuid(0x15, 0);
    if (freqs.ebx != 0 and freqs.ecx != 0) {
        hertz = freqs.ecx * (freqs.ebx / freqs.eax);
        return .success;
    }
    const before = rdtsc();
    timers.sleep(CALIBRATION_NS);
    const after = rdtsc();
    hertz = (after - before) * (1_000_000_000 / CALIBRATION_NS);
    return .success;
}

pub fn rdtsc() usize {
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
    return rdtsc() * 1_000_000_000 / hertz;
}
