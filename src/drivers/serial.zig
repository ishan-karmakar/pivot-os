pub fn out(port: u16, val: anytype) void {
    switch (@TypeOf(val)) {
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
        else => @compileError("Unknown size used"),
    }
}

pub fn in(port: u16, T: type) T {
    switch (T) {
        u32 => return asm volatile ("in %%dx, %%eax"
            : [val] "={eax}" (-> T),
            : [port] "{dx}" (port),
        ),
        u16 => return asm volatile ("in %%dx, %%ax"
            : [val] "={ax}" (-> T),
            : [port] "{dx}" (port),
        ),
        u8 => return asm volatile ("in %%dx, %%al"
            : [val] "={al}" (-> T),
            : [port] "{dx}" (port),
        ),
        else => @compileError("Unknown size used"),
    }
}
