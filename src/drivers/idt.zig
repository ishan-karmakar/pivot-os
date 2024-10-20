const Entry = packed struct {
    off0: u16,
    seg: u16 = 0x8,
    ist: u8 = 0,
    flags: u8 = 0x8E,
    off1: u16,
    off2: u32,
    rsv: u32 = 0,
};

const IDTR = packed struct {
    size: u16,
    addr: usize,
};

var idtr = IDTR{
    .size = 256 * @sizeOf(Entry) - 1,
    .addr = 0,
};

var table: [256]Entry = undefined;

var fn_table = create_fn_table();

pub fn init() void {
    for (0..256) |i| {}
}

fn create_handler(comptime int_num: usize) fn () void {
    _ = int_num;
    const T = struct {
        fn handler() void {}
    };
    return T.handler;
}

fn create_fn_table() [256]usize {
    var t: [256]fn () void = undefined;
    for (0..256) |i| {
        t[i] = @intFromPtr(&create_handler(i));
    }
    return t;
}
