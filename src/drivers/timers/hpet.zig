const kernel = @import("kernel");
const std = @import("std");
const uacpi = @import("uacpi");
const log = std.log.scoped(.hpet);
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

    pub inline fn is_irq_supported(self: @This(), _irq: u5) bool {
        return (self.config_cap.int_route_cap & (@as(u32, 1) << _irq)) != 0;
    }
};

var registers: *volatile Registers = undefined;

pub var HPETCommonTask = kernel.Task{
    .name = "HPET Common",
    .init = init_common,
    .dependencies = &.{
        .{ .task = &kernel.lib.mem.KMapperTask },
        .{ .task = &kernel.drivers.acpi.TablesTask },
    },
};

pub const TimerVTable = timers.TimerVTable{
    .requires_calibration = false,
    .callback = timer_callback,
};

pub var TimerTask = kernel.Task{
    .name = "HPET Timer",
    .init = init_timer,
    .dependencies = &.{
        .{ .task = &HPETCommonTask },
        .{ .task = &kernel.drivers.idt.Task },
        .{ .task = &kernel.drivers.intctrl.Task },
    },
};

pub const GTSVTable = timers.GTSVTable{
    .requires_calibration = false,
    .time = time,
};

pub var GTSTask = kernel.Task{
    .name = "HPET GTS",
    .init = null,
    .dependencies = &.{
        .{ .task = &HPETCommonTask },
    },
};

var callback: timers.CallbackFn = undefined;
var irq: usize = undefined;
var handler: *idt.HandlerData = undefined;
var comp: *Comparator = undefined;

// FIXME: Explore FSB interrupts (don't think QEMU/VirtualBox supports them, but my physical machine does)
// FIXME: Use standard mapping if leg ret doesn't exist
// TODO: Multiple comparators (for scheduling on different CPUs, periodic not necessary)
fn init_common() kernel.Task.Ret {
    const hpet = acpi.get_table(uacpi.acpi_hpet, uacpi.ACPI_HPET_SIGNATURE) orelse {
        log.debug("HPET not found", .{});
        return .failed;
    };
    registers = @ptrFromInt(hpet.address.address);
    map_hpet();
    registers.gcfg.enable_cnf = true;

    return .success;
}

fn init_timer() kernel.Task.Ret {
    if (!registers.gcap_id.leg_rt_cap) return .failed;
    registers.gcfg.leg_rt_cnf = true;
    comp = registers.get_comparator(0);
    log.debug("Supports FSB interrupts: {}", .{comp.config_cap.fsb_int_del_cap});
    handler = idt.allocate_handler(intctrl.pref_vec(0));
    irq = intctrl.map(idt.handler2vec(handler), 0) catch {
        handler.reserved = false;
        return .failed;
    };
    handler.handler = timer_handler;

    return .success;
}

// TODO: Only map main area and specific comparator we are using
fn map_hpet() void {
    mem.kmapper.map(@intFromPtr(registers), @intFromPtr(registers), 0b11);
    const comparator_start = @intFromPtr(registers.get_comparator(0));
    const comparator_end = @intFromPtr(registers.get_comparator(registers.gcap_id.num_tim_cap)) + @sizeOf(Comparator);
    var comparator_page_start = @divFloor(comparator_start, 0x1000) * 0x1000;
    const comparator_page_end = @divFloor(comparator_end, 0x1000) * 0x1000;
    while (comparator_page_start <= comparator_page_end) {
        mem.kmapper.map(comparator_page_start, comparator_page_start, 0b11);
        comparator_page_start += 0x1000;
    }
}

fn timer_callback(ns: usize, ctx: ?*anyopaque, _handler: timers.CallbackFn) void {
    const ticks = ns * 1_000_000 / registers.gcap_id.counter_clk_period;
    callback = _handler;
    handler.ctx = ctx;

    registers.gcfg.enable_cnf = false;
    comp.comparator = registers.counter + ticks;
    comp.config_cap.int_enb_cnf = true;
    registers.gcfg.enable_cnf = true;
    intctrl.mask(irq, false);
}

/// Returns the HPET counter value in nanoseconds
fn time() usize {
    return registers.counter * registers.gcap_id.counter_clk_period / 1_000_000;
}

fn timer_handler(ctx: ?*anyopaque, status: *cpu.Status) *const cpu.Status {
    const ret = callback(ctx, status);
    intctrl.eoi(irq);
    return ret;
}
