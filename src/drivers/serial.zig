pub fn out(port: u16, val: anytype) !void {
    return switch (@TypeOf(val)) {
        u32 => asm volatile ("out %%eax, %%dx"
            :
            : [port] "{dx}" (port),
              [val] "{eax}" (val),
        ),
        u16 => asm volatile ("out %%ax, %%dx"
            :
            : [port] "{dx}" (port),
              [val] "{ax}" (val),
        ),
        u8 => asm volatile ("out %%al, %%dx"
            :
            : [port] "{dx}" (port),
              [val] "{al}" (val),
        ),
        else => error.UnknownSize,
    };
}
