// pub fn create(port: u16, comptime T: type) type {
//     return struct {
//         pub fn out(val: T) void {
//             asm volatile ("outb %al, %dx"
//                 :
//                 : [port] "{dx}" (port),
//                   [val] "{al}" (val),
//             );
//         }

//         pub fn in() T {
//             return asm volatile ("inb %[ret], dx"
//                 : [ret] "{al}" (-> u8),
//                 : [port] "{dx}" (port),
//             );
//         }
//     };
// }

pub fn out(port: u16, val: anytype) !void {
    return switch (@TypeOf(val)) {
        u16 => asm volatile ("out %%ax, %%dx"
            :
            : [port] "{dx}" (port),
              [val] "{al}" (val),
        ),
        u8 => asm volatile ("out %%al, %%dx"
            :
            : [port] "{dx}" (port),
              [val] "{al}" (val),
        ),
        else => error.UnknownSize,
    };
}
