const std = @import("std");
const kernel = @import("root");
const log = std.log.scoped(.lwip);

// export fn memmove(dest: [*c]u8, src: [*c]const u8, num: usize) ?*anyopaque {
//     @memmove(dest, src[0..num]);
//     return dest;
// }

export fn strncmp(s1: [*c]const u8, s2: [*c]const u8, num: usize) c_int {
    log.info("strncmp()", .{});
    const slice1 = std.mem.sliceTo(s1, 0);
    const slice2 = std.mem.sliceTo(s2, 0);

    const n = @min(num, @min(slice1.len, slice2.len));
    return switch (std.mem.order(u8, slice1[0..n], slice2[0..n])) {
        .lt => -1,
        .eq => if (n < num and slice1.len != slice2.len)
            if (slice1.len < slice2.len) -1 else 1
        else
            0,
        .gt => 1,
    };
}

export fn strlen(str: [*c]const u8) usize {
    log.info("strlen()", .{});
    return std.mem.len(str);
}

// export fn memcpy(dest: [*c]u8, src: [*c]const u8, len: usize) ?*anyopaque {
//     @memcpy(dest, src[0..len]);
//     return dest;
// }

// export fn memcmp(s1: [*c]const u8, s2: [*c]const u8, len: usize) c_int {
//     return switch (std.mem.order(u8, s1[0..len], s2[0..len])) {
//         .lt => -1,
//         .eq => 0,
//         .gt => 1,
//     };
// }

// export fn memset(dest: [*c]u8, val: c_int, len: usize) ?*anyopaque {
//     @memset(dest[0..len], @intCast(val));
//     return dest;
// }

export fn sys_now() u32 {
    // Convert NS -> MS
    return @intCast(kernel.drivers.timers.time() / 1_000_000);
}
