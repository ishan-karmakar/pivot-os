const log = @import("std").log.scoped(.hpet);
const uacpi = @import("uacpi");
const mem = @import("kernel").lib.mem;
const cpu = @import("kernel").drivers.cpu;
const ioapic = @import("kernel").drivers.ioapic;
const idt = @import("kernel").drivers.idt;

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

    pub inline fn timer_config_cap(self: *const volatile @This(), n: u32) *TimerConfigurationCapability {
        return @ptrFromInt(@intFromPtr(self) + 0x100 + 0x20 * n);
    }

    pub inline fn timer_comparator_val(self: *const volatile @This(), n: u32) *u64 {
        return @ptrFromInt(@intFromPtr(self) + 0x108 + 0x20 * n);
    }

    pub inline fn timer_fsb_int_route(self: *const volatile @This(), n: u32) *u64 {
        return @ptrFromInt(@intFromPtr(self) + 0x110 + 0x20 * n);
    }
};

var registers: *volatile Registers = undefined;
var triggered: bool = false;
const HPET_VEC = 0x20;

pub fn init() void {
    var tbl: uacpi.uacpi_table = undefined;
    if (uacpi.uacpi_table_find_by_signature("HPET", &tbl) != uacpi.UACPI_STATUS_OK) {
        @panic("Could not find HPET table. Non HPET support unimplemented");
    }

    const hpet_tbl: *const uacpi.acpi_hpet = @ptrCast(tbl.unnamed_0.hdr);
    registers = @ptrFromInt(hpet_tbl.address.address);
    map_hpet();
    registers.gcfg.enable_cnf = true;
    idt.set_ent(HPET_VEC, idt.create_irq(HPET_VEC, "hpet_timer_handler"));
    for (0..32) |i| {
        const irq: u5 = @truncate(i);
        if ((registers.timer_config_cap(0).int_route_cap & (@as(u32, 1) << irq)) == 0) continue;
        registers.timer_config_cap(0).int_route_cnf = irq;
        ioapic.set(HPET_VEC, irq, 0, 0);
        ioapic.mask(irq, false);
        break;
    }
    log.info("Initialized HPET timer (ticks occur every {} femtoseconds)", .{registers.gcap_id.counter_clk_period});
}

fn map_hpet() void {
    mem.kmapper.map(@intFromPtr(registers), @intFromPtr(registers), 0b10);
    const comparator_start = @intFromPtr(registers.timer_config_cap(0));
    const comparator_end = @intFromPtr(registers.timer_fsb_int_route(registers.gcap_id.num_tim_cap)) + @sizeOf(u64);
    var comparator_page_start = @divFloor(comparator_start, 0x1000) * 0x1000;
    const comparator_page_end = @divFloor(comparator_end, 0x1000) * 0x1000;
    while (comparator_page_start <= comparator_page_end) {
        mem.kmapper.map(comparator_page_start, comparator_page_start, 0b11);
        comparator_page_start += 0x1000;
    }
}

/// Returns current time in femtoseconds
pub inline fn time() usize {
    return registers.counter;
}

/// Start HPET timer in nonperiodic mode
/// If interval < COUNTER_CLK_PERIOD, interval = COUNTER_CLK_PERIOD
fn start(ns: usize) void {
    asm volatile ("sti");
    registers.timer_config_cap(0).int_enb_cnf = true;
    log.info("{}", .{registers.timer_config_cap(0).int_route_cnf});
    registers.timer_comparator_val(0).* = registers.counter + (ns * 1_000_000 / registers.gcap_id.counter_clk_period);
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

pub fn nsleep(ns: usize) void {
    start(ns);
    // while (!triggered) {
    //     // asm volatile ("pause");
    // }
}

export fn hpet_timer_handler(status: *const cpu.Status, _: usize) *const cpu.Status {
    log.info("hpet_timer_handler", .{});
    triggered = true;
    return status;
}
