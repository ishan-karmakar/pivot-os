const kernel = @import("kernel");
const VTable = kernel.drivers.timers.VTable;
const hpet = kernel.drivers.timers.hpet;

pub const vtable: VTable = .{
    .init = init,
    .time = time,
    .sleep = sleep,
    .set_periodic = null,
    .set_oneshot = null,
};

var ticks_per_ns: usize = undefined;

fn init() bool {
    var cal_timer: *const VTable = undefined;
    if (hpet.vtable.init()) {
        cal_timer = &hpet.vtable;
    } else return false;
    // TODO: Check for invariant TSC
    const before = raw_time();
    cal_timer.sleep(50000000); // 50 ms
    const after = raw_time();
    ticks_per_ns = (after - before) / 50000000;
    return true;
}

fn raw_time() usize {
    var upper: u32 = undefined;
    var lower: u32 = undefined;
    asm volatile ("rdtsc"
        : [edx] "={edx}" (upper),
          [eax] "={eax}" (lower),
    );
    return (@as(u64, @intCast(upper)) << 32) | lower;
}

fn time() usize {
    @panic("tsc time() unimplemented");
}

fn sleep(_: usize) void {
    @panic("tsc sleep() unimplemented");
}
