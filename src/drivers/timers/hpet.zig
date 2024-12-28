const kernel = @import("kernel");
const log = @import("std").log.scoped(.hpet);
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

    pub inline fn is_irq_supported(self: @This(), irq: u5) bool {
        return (self.config_cap.int_route_cap & (@as(u32, 1) << irq)) != 0;
    }
};

pub const vtable: timers.VTable = .{
    .init = init,
    .time = time,
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
    initialized = true;
    _ = timers.pit.vtable.init();
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

fn get_free_comparator(periodic: bool) ?*Comparator {
    const num_comp = registers.gcap_id.num_tim_cap + 1;
    for (0..num_comp) |i| {
        const comp = registers.get_comparator(i);
        if (!comp.config_cap.int_enb_cnf) {
            if (periodic) {
                if (comp.config_cap.per_int_cap) return comp;
            } else {
                return comp;
            }
        }
    }
    return null;
}

fn start_oneshot(ns: usize, info: *HandlerInfo) *idt.HandlerData {
    _ = ns;
    _ = info;
    // const comparator = get_free_comparator(false);
    // const delta = ns * 1_000_000 / registers.gcap_id.counter_clk_period;
    const handler = idt.allocate_handler(null);
    // handler.ctx = info;
    // handler.handler = timer_handler;
    // intctrl.set(idt.handler2vec(handler), )

    // const comparator = registers.get_comparator(0);
    // const delta = ns * 1_000_000 / registers.gcap_id.counter_clk_period;
    // const handler = idt.allocate_handler(0x20);
    // handler.ctx = info;
    // handler.handler = timer_handler;
    // intctrl.set(idt.handler2vec(handler), 0, 0);
    // registers.gcfg.enable_cnf = false;
    // comparator.config_cap.int_enb_cnf = true;
    // comparator.comparator = registers.counter + delta;
    // registers.gcfg.enable_cnf = true;
    return handler;
}

fn sleep(ns: usize) void {
    var info = HandlerInfo{};
    const comp: *Comparator = get_free_comparator(false) orelse @panic("Out of free comparators");
    const delta = ns * 1_000_000 / registers.gcap_id.counter_clk_period;
    var irq: ?usize = null;
    var handler: *idt.HandlerData = undefined;
    for (0..32) |i| {
        if (comp.is_irq_supported(@intCast(i))) {
            handler = idt.allocate_handler(@intCast(i + 0x20));
            // FIXME: Panic if vector actually given is not appropriate for 8259 PIC
            const ret = intctrl.controller.map(idt.handler2vec(handler), i);
            if (ret == error.IRQUsed) {
                handler.reserved = false;
                continue;
            }

            irq = ret catch @panic("Error mapping HPET comparator IRQ");
            handler.handler = timer_handler;
            handler.ctx = &info;
        }
    }
    if (irq == null) @panic("No suitable IRQ found for HPET comparator");
    registers.gcfg.enable_cnf = false;
    comp.config_cap.int_enb_cnf = true;
    comp.comparator = registers.counter + delta;
    registers.gcfg.enable_cnf = true;
    intctrl.controller.mask(irq.?, false);
    while (!@atomicLoad(bool, @as(*volatile bool, &info.triggered), .unordered)) asm volatile ("pause");
    intctrl.controller.unmap(irq.?);
    handler.reserved = false;
}

const HandlerInfo = struct {
    triggered: bool = false,
};

fn timer_handler(ctx: ?*anyopaque, status: *const cpu.Status) *const cpu.Status {
    const info: *HandlerInfo = @alignCast(@ptrCast(ctx));
    info.triggered = true;
    intctrl.controller.eoi(0);
    return status;
}
