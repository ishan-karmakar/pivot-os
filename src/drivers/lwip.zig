const kernel = @import("kernel");
const mem = kernel.lib.mem;
const timers = kernel.drivers.timers;
const std = @import("std");
const lwip = @import("lwip");

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
export fn sys_arch_protect(_: lwip.sys_prot_t) lwip.sys_prot_t {
    asm volatile ("cli");
    return 0;
}

export fn sys_arch_unprotect(_: lwip.sys_prot_t) void {
    asm volatile ("sti");
}

export fn sys_now() u32 {
    return @intCast(timers.time() / 1_000_000);
}

export fn sys_sem_new(_: *lwip.sys_sem_t, _: u8) lwip.err_t {
    @panic("sys_sem_new()");
}

export fn sys_arch_sem_wait(_: *lwip.sys_sem_t, _: u32) u32 {
    @panic("sys_arch_sem_wait()");
}

export fn sys_init() void {}

export fn sys_sem_free(_: *lwip.sys_sem_t) void {
    @panic("sys_sem_free()");
}

export fn sys_mbox_valid(_: *lwip.sys_mbox_t) c_int {
    @panic("sys_mbox_valid()");
}

export fn sys_mbox_trypost(_: *lwip.sys_mbox_t, _: *anyopaque) lwip.err_t {
    @panic("sys_mbox_trypost()");
}

export fn sys_mbox_post(_: *lwip.sys_mbox_t, _: *anyopaque) void {
    @panic("sys_mbox_post()");
}

export fn sys_mbox_trypost_fromisr(_: *lwip.sys_mbox_t, _: *anyopaque) lwip.err_t {
    @panic("sys_mbox_trypost_fromisr()");
}

export fn sys_mbox_new(_: *lwip.sys_mbox_t, _: c_int) lwip.err_t {
    @panic("sys_mbox_new()");
}

export fn sys_arch_mbox_fetch(_: *lwip.sys_mbox_t, _: **anyopaque, _: u32) u32 {
    @panic("sys_arch_mbox_fetch()");
}

export fn sys_mutex_new(_: *lwip.sys_mutex_t) lwip.err_t {
    @panic("sys_mutex_new()");
}

export fn sys_mutex_lock(_: *lwip.sys_mutex_t) void {
    @panic("sys_mutex_lock()");
}

export fn sys_mutex_unlock(_: *lwip.sys_mutex_t) void {
    @panic("sys_mutex_unlock");
}

export fn sys_thread_new(_: [*c]const u8, _: lwip.lwip_thread_fn, _: *anyopaque, _: c_int, _: c_int) lwip.sys_thread_t {
    @panic("sys_thread_new()");
}
