const log = @import("std").log.scoped(.hpet);
const uacpi = @import("uacpi");
const mem = @import("kernel").lib.mem;

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

    pub inline fn timer_config_cap(self: *const @This(), n: u32) *TimerConfigurationCapability {
        return @ptrFromInt(@intFromPtr(self) + 0x100 + 0x20 * n);
    }

    pub inline fn timer_comparator_val(self: *const @This(), n: u32) *u64 {
        return @ptrFromInt(@intFromPtr(self) + 0x108 + 0x20 * n);
    }

    pub inline fn timer_fsb_int_route(self: *const @This(), n: u32) *u64 {
        return @ptrFromInt(@intFromPtr(self) + 0x110 + 0x20 * n);
    }
};

pub fn init() void {
    var tbl: uacpi.uacpi_table = undefined;
    if (uacpi.uacpi_table_find_by_signature("HPET", &tbl) != uacpi.UACPI_STATUS_OK) {
        @panic("Could not find HPET table. Non HPET support unimplemented");
    }

    const hpet_tbl: *const uacpi.acpi_hpet = @ptrCast(tbl.unnamed_0.hdr);
    const registers: *Registers = @ptrFromInt(hpet_tbl.address.address);
    mem.kmapper.map(hpet_tbl.address.address, hpet_tbl.address.address, 0b10);
    log.info("{}", .{registers.timer_config_cap(0).mode32_cnf});
    log.info("Initialized HPET timer", .{});
}

// GCAP + ID
// - 1 timer
// - Can operate in 64 bit mode
// - Can use legacy replacement mapping
// - 10000000 femtoseconds ticks - 10 nanoseconds
// - Vendor ID: 32902
// - Revision ID: 1

// Timer supports periodic mode
