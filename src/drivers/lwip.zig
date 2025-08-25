const std = @import("std");
const kernel = @import("root");
const log = std.log.scoped(.lwip);
const lwip = @import("lwip");

pub var Task = kernel.Task{
    .name = "LWIP",
    .init = init,
    .dependencies = &.{
        .{ .task = &kernel.drivers.timers.Task },
    },
};

// export fn memmove(dest: [*c]u8, src: [*c]const u8, num: usize) ?*anyopaque {
//     @memmove(dest, src[0..num]);
//     return dest;
// }

fn init() kernel.Task.Ret {
    lwip.lwip_init();
    var netif = lwip.netif{};
    var ipaddr = lwip.ip4_addr_t{};
    var netmask = lwip.ip4_addr_t{};
    var gw = lwip.ip4_addr_t{};
    _ = lwip.netif_add(&netif, &ipaddr, &netmask, &gw, null, if_init, lwip.ethernet_input);
    return .success;
}

fn if_init(netif: [*c]lwip.netif) callconv(.c) lwip.err_t {
    _ = netif;
    @panic("if_init()");
}

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

export fn sys_arch_protect() c_char {
    asm volatile ("cli");
    return 0;
}

export fn sys_arch_unprotect(_: c_char) void {
    asm volatile ("sti");
}

export fn sys_init() void {}

export fn sys_sem_new(sem: *std.atomic.Value(u8), count: u8) lwip.err_t {
    sem.* = std.atomic.Value(u8).init(count);
    return lwip.ERR_OK;
}

// export fn sys_mutex_new(mtx: *lwip.sys_mutex_t) lwip.err_t {
//     _ = mtx;
//     @panic("sys_mutex_new");
// }

// export fn sys_mutex_lock(mtx: *lwip.sys_mutex_t) void {
//     _ = mtx;
//     @panic("sys_mutex_lock");
// }

// export fn sys_mutex_unlock(mtx: *lwip.sys_mutex_t) void {
//     _ = mtx;
//     @panic("sys_mutex_unlock");
// }

export fn sys_arch_sem_wait(sem: *lwip.sys_sem_t, timeout: u32) u32 {
    _ = sem;
    _ = timeout;
    @panic("sys_arch_sem_wait");
}

export fn sys_sem_free(sem: *lwip.sys_sem_t) void {
    _ = sem;
    @panic("sys_sem_free");
}

export fn sys_mbox_valid(mbox: *lwip.sys_mbox_t) c_int {
    _ = mbox;
    @panic("sys_mbox_valid");
}

export fn sys_mbox_trypost(mbox: *lwip.sys_mbox_t, msg: ?*anyopaque) lwip.err_t {
    _ = mbox;
    _ = msg;
    @panic("sys_mbox_trypost");
}

export fn sys_mbox_post(mbox: *lwip.sys_mbox_t, msg: ?*anyopaque) void {
    _ = mbox;
    _ = msg;
    @panic("sys_mbox_post");
}

export fn sys_mbox_trypost_fromisr(mbox: *lwip.sys_mbox_t, msg: ?*anyopaque) lwip.err_t {
    _ = mbox;
    _ = msg;
    @panic("sys_mbox_trypost");
}

export fn sys_mbox_new(mbox: *lwip.sys_mbox_t, size: c_int) lwip.err_t {
    _ = mbox;
    _ = size;
    @panic("sys_mbox_new");
}

export fn sys_thread_new(name: [*c]const u8, thread: lwip.lwip_thread_fn, arg: ?*anyopaque, stacksize: c_int, prio: c_int) lwip.sys_thread_t {
    _ = name;
    _ = thread;
    _ = arg;
    _ = stacksize;
    _ = prio;
    @panic("sys_thread_new");
}

export fn sys_arch_mbox_fetch(mbox: *lwip.sys_mbox_t, msg: ?**anyopaque, timeout: u32) u32 {
    _ = mbox;
    _ = msg;
    _ = timeout;
    @panic("sys_arch_mbox_fetch");
}

export fn sys_sem_signal(sem: *lwip.sys_sem_t) void {
    _ = sem;
    @panic("sys_sem_signal");
}

export fn lwip_assert(str: [*c]const u8) void {
    log.info("{s}", .{str});
}
