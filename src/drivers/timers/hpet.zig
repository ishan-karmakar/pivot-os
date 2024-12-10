const kernel = @import("kernel");
const log = @import("std").log.scoped(.hpet);
const ArrayList = @import("std").ArrayList;
const acpi = kernel.drivers.acpi;
const mem = kernel.lib.mem;
const cpu = kernel.drivers.cpu;
const timers = kernel.drivers.timers;
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

pub const vtable: timers.VTable = .{
    .capabilities = timers.CAPABILITIES_COUNTER | timers.CAPABILITIES_IRQ,
    .init = init,
    .time = time,
    // .set_oneshot = set_oneshot,
    .sleep = sleep,
};

var initialized: ?bool = null;
var registers: *Registers = undefined;

fn init() bool {
    // return false;
    defer initialized = initialized orelse false;
    if (initialized) |i| return i;
    const tbl = acpi.hpet orelse return false;
    registers = @ptrFromInt(tbl.address.address);
    map_hpet();
    // TODO: Debug why standard mapping doesn't work with -enable-kvm
    if (!registers.gcap_id.leg_rt_cap) {
        log.debug("Legacy replacement mapping not supported", .{});
        return false;
    }
    registers.gcfg.leg_rt_cnf = true;
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

fn start_oneshot(ns: usize, info: *HandlerInfo) u8 {
    const comparator = registers.get_comparator(0);
    const delta = ns * 1_000_000 / registers.gcap_id.counter_clk_period;
    const vec = idt.allocate_vec(0x20, timer_handler, info) orelse @panic("Out of interrupt handlers");
    intctrl.set(vec, 0, 0);
    registers.gcfg.enable_cnf = false;
    // comparator.config_cap.int_route_cnf = irq;
    comparator.config_cap.int_enb_cnf = true;
    comparator.comparator = registers.counter + delta;
    registers.gcfg.enable_cnf = true;
    info.irq = 0;
    return vec;
}

fn sleep(ns: usize) void {
    var info = HandlerInfo{};
    const vec = start_oneshot(ns, &info);
    intctrl.mask(info.irq, false);
    // Volatile to make sure that compiler keeps checking memory location
    // Atomic to make sure that interrupt doesn't cut through load
    while (!@atomicLoad(bool, @as(*volatile bool, &info.triggered), .unordered)) asm volatile ("pause");
    intctrl.mask(info.irq, true);
    idt.free_vec(vec);
}

const HandlerInfo = struct {
    triggered: bool = false,
    irq: u5 = 0,
};

fn timer_handler(ctx: ?*anyopaque, status: *const cpu.Status) *const cpu.Status {
    log.info("hpet timer handler", .{});
    const info: *HandlerInfo = @alignCast(@ptrCast(ctx));
    info.triggered = true;
    intctrl.eoi(info.irq);
    return status;
}
