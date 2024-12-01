const kernel = @import("kernel");
const log = @import("std").log.scoped(.hpet);
const ArrayList = @import("std").ArrayList;
const acpi = kernel.drivers.acpi;
const mem = kernel.lib.mem;
const cpu = kernel.drivers.cpu;
const VTable = kernel.drivers.timers.VTable;
const intctrl = kernel.drivers.intctrl;
const idt = kernel.drivers.idt;

const GeneralCapabilitiesID = packed struct {
    rev_id: u8,
    num_tim_cap: u5,
    count_size_cap: bool,
    rsv: u1,
    leg_rt_cap: bool,
    vendor_id: u16,
    counter_clk_period: u32,
};

const GeneralConfiguration = packed struct {
    enable_cnf: bool,
    leg_rt_cnf: bool,
    rsv: u62,
};

const TimerConfigurationCapability = packed struct {
    rsv0: u1,
    int_type_cnf: bool,
    int_enb_cnf: bool,
    type_cnf: bool,
    per_int_cap: bool,
    size_cap: bool,
    val_set_cnf: bool,
    rsv1: u1,
    mode32_cnf: bool,
    int_route_cnf: u5,
    fsb_en_cnf: bool,
    fsb_int_del_cap: bool,
    rsv2: u16,
    int_route_cap: u32,
};

const Registers = packed struct {
    gcap_id: GeneralCapabilitiesID,
    rsv0: u64,
    gcfg: GeneralConfiguration,
    rsv1: u64,
    gisr: u64,
    rsv2: u1600,
    counter: u64,

    pub inline fn get_comparator(self: *const volatile @This(), n: usize) *Comparator {
        return @ptrFromInt(@intFromPtr(self) + 0x100 + 0x20 * n);
    }
};

const Comparator = packed struct {
    config_cap: TimerConfigurationCapability,
    comparator: u64,
    fsb_int_route: u64,

    pub fn get_first_irq(self: @This()) u5 {
        for (0..32) |i| if ((self.config_cap.int_route_cap & (@as(u32, 1) << @intCast(i))) > 0) return @intCast(i);
        @panic("No IRQ capable for comparator");
    }
};

pub const vtable: VTable = .{
    .init = init,
    .time = time,
    .set_oneshot = set_oneshot,
    .set_periodic = set_periodic,
    .sleep = sleep,
};

var initialized: bool = false;
var registers: *volatile Registers = undefined;

fn init() bool {
    if (initialized) return true;
    const tbl = acpi.hpet orelse return false;
    kernel.drivers.timers.pit.disable();
    registers = @ptrFromInt(tbl.address.address);
    map_hpet();
    initialized = true;
    log.info("HPET timer initialized", .{});
    return true;
}

fn map_hpet() void {
    mem.kmapper.map(@intFromPtr(registers), @intFromPtr(registers), 0b10);
    const comparator_start = @intFromPtr(registers.get_comparator(0));
    const comparator_end = @intFromPtr(registers.get_comparator(registers.gcap_id.num_tim_cap)) + @sizeOf(Comparator);
    var comparator_page_start = @divFloor(comparator_start, 0x1000) * 0x1000;
    const comparator_page_end = @divFloor(comparator_end, 0x1000) * 0x1000;
    while (comparator_page_start <= comparator_page_end) {
        mem.kmapper.map(comparator_page_start, comparator_page_start, 0b10);
        comparator_page_start += 0x1000;
    }
}

/// Returns the HPET counter value in nanoseconds
fn time() usize {
    // TODO: Cache counter_clk_period
    return registers.counter * registers.gcap_id.counter_clk_period / 1_000_000;
}

fn sleep(ns: usize) void {
    const comparator = registers.get_comparator(0);
    const irq = comparator.get_first_irq();
    const delta = ns * 1_000_000 / registers.gcap_id.counter_clk_period;
    var info = HandlerInfo{ .irq = irq };
    const vec = idt.allocate(timer_handler, &info) orelse @panic("Out of interrupt handlers");
    intctrl.set(vec, irq, 0);
    registers.gcfg.enable_cnf = false;
    comparator.config_cap.int_route_cnf = irq;
    comparator.config_cap.int_enb_cnf = true;
    comparator.comparator = registers.counter + delta;
    registers.gcfg.enable_cnf = true;
    intctrl.mask(irq, false);
    while (!info.triggered) asm volatile ("pause");
    intctrl.mask(irq, true);
    idt.free_vecs(vec, vec);
}

/// Start HPET timer in nonperiodic mode
/// If interval < COUNTER_CLK_PERIOD, interval = COUNTER_CLK_PERIOD
fn start(ns: usize) void {
    registers.gcfg.enable_cnf = false;
    intctrl.mask(registers.timer_config_cap(0).int_route_cnf, false);
    registers.timer_config_cap(0).int_enb_cnf = true;
    registers.timer_comparator_val(0).* = registers.counter + (ns * 1_000_000 / registers.gcap_id.counter_clk_period);
    registers.gcfg.enable_cnf = true;
}

fn set_oneshot(_: usize, _: *const fn () void) bool {
    registers.gcfg.enable_cnf = false;
    @panic("set_oneshot unimplemented");
}

fn set_periodic(_: usize, _: *const fn () void) bool {
    @panic("set_periodic unimplemented");
}

fn start_periodic(fs: usize) void {
    for (0..registers.gcap_id.num_tim_cap) |i| {
        if (!registers.timer_config_cap(i).per_int_cap) continue;
        registers.timer_config_cap(i).type_cnf = true;
        registers.timer_config_cap(i).val_set_cnf = true;
        registers.timer_comparator_val(i).* = registers.counter + fs;
        registers.timer_comparator_val(i).* = fs;
        return;
    }
    log.warn("Failed to find HPET comparator that supports periodic mode", .{});
}

const HandlerInfo = struct {
    triggered: bool = false,
    irq: u5,
};
fn timer_handler(ctx: ?*anyopaque, status: *const cpu.Status) *const cpu.Status {
    // log.info("hpet_timer_handler, {}, {}", .{ registers.counter, registers.get_comparator(0).comparator });
    log.info("hpet_timer_handler", .{});
    const info: *HandlerInfo = @alignCast(@ptrCast(ctx));
    info.triggered = true;
    intctrl.eoi(info.irq);
    return status;
}
