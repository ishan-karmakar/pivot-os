const kernel = @import("kernel");
const mem = kernel.lib.mem;
const timers = kernel.drivers.timers;
const std = @import("std");

// LWIP doesn't support sized frees, so we have to encode the size within the malloced buffer
export fn lwip_malloc(_size: c_ulong) *anyopaque {
    const size = _size + @sizeOf(c_ulong);
    const ptr = @as([]c_ulong, @ptrCast(mem.kheap.allocator().alignedAlloc(u8, std.mem.Alignment.@"8", size + @sizeOf(c_ulong)) catch @panic("OOM")));
    ptr[0] = size;
    return @ptrCast(&ptr[1]);
}

export fn lwip_free(ptr: *anyopaque) void {
    const buf: [*]c_ulong = @ptrFromInt(@intFromPtr(ptr) - @sizeOf(c_ulong));
    const size = buf[0];
    mem.kheap.allocator().free(buf[0..size]);
}

export fn lwip_calloc(size: c_ulong) *anyopaque {
    const buf = lwip_malloc(size);
    @memset(@as([*]u8, @ptrCast(buf))[0..size], 0);
    return buf;
}

// FIXME: Change to a mutex? Still haven't thought about implications of disabling interrupts here
export fn sys_arch_protect(_: u8) u8 {
    asm volatile ("cli");
    return 0;
}

export fn sys_arch_unprotect(_: u8) void {
    asm volatile ("sti");
}

export fn sys_now() u32 {
    return @intCast(timers.time() / 1_000_000);
}
