const smp = @import("kernel").drivers.smp;

pub const TSS = packed struct {
    rsv0: u32,
    rsp0: u64,
    rsp1: u64,
    rsp2: u64,
    rsv1: u64,
    ist1: u64,
    ist2: u64,
    ist3: u64,
    ist4: u64,
    ist5: u64,
    ist6: u64,
    ist7: u64,
    rsv2: u64,
    rsv3: u64,
    iopb: u16,
};

pub fn init() void {}
