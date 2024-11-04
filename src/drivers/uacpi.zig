const uacpi = @import("uacpi");
const serial = @import("kernel").drivers.serial;
const mem = @import("kernel").lib.mem;
const timers = @import("kernel").drivers.timers;
const log = @import("std").log.scoped(.uacpi);

export fn uacpi_kernel_raw_memory_read(addr: uacpi.uacpi_phys_addr, bw: uacpi.uacpi_u8, out: [*c]uacpi.uacpi_u64) uacpi.uacpi_status {
    out.* = switch (bw) {
        1 => @intCast(@as(*const u8, @ptrFromInt(addr)).*),
        2 => @intCast(@as(*const u16, @ptrFromInt(addr)).*),
        4 => @intCast(@as(*const u32, @ptrFromInt(addr)).*),
        8 => @as(*const u64, @ptrFromInt(addr)).*,
        else => return uacpi.UACPI_STATUS_TYPE_MISMATCH,
    };
    return uacpi.UACPI_STATUS_OK;
}

export fn uacpi_kernel_raw_memory_write(addr: uacpi.uacpi_phys_addr, bw: uacpi.uacpi_u8, val: uacpi.uacpi_u64) uacpi.uacpi_status {
    switch (bw) {
        1 => @as(*u8, @ptrFromInt(addr)).* = @truncate(val),
        2 => @as(*u16, @ptrFromInt(addr)).* = @truncate(val),
        4 => @as(*u32, @ptrFromInt(addr)).* = @truncate(val),
        8 => @as(*u64, @ptrFromInt(addr)).* = @truncate(val),
        else => return uacpi.UACPI_STATUS_TYPE_MISMATCH,
    }
    return uacpi.UACPI_STATUS_OK;
}

export fn uacpi_kernel_raw_io_read(_addr: uacpi.uacpi_io_addr, bw: uacpi.uacpi_u8, out: [*c]uacpi.uacpi_u64) uacpi.uacpi_status {
    const addr: u16 = @intCast(_addr);
    out.* = switch (bw) {
        1 => serial.in(addr, u8),
        2 => serial.in(addr, u16),
        4 => serial.in(addr, u32),
        else => return uacpi.UACPI_STATUS_TYPE_MISMATCH,
    };
    return uacpi.UACPI_STATUS_OK;
}

export fn uacpi_kernel_raw_io_write(_addr: uacpi.uacpi_io_addr, bw: uacpi.uacpi_u8, val: uacpi.uacpi_u64) uacpi.uacpi_status {
    const addr: u16 = @intCast(_addr);
    switch (bw) {
        1 => serial.out(addr, @as(u8, @truncate(val))),
        2 => serial.out(addr, @as(u16, @truncate(val))),
        4 => serial.out(addr, @as(u32, @truncate(val))),
        else => return uacpi.UACPI_STATUS_TYPE_MISMATCH,
    }
    return uacpi.UACPI_STATUS_OK;
}

export fn uacpi_kernel_io_map(_: uacpi.uacpi_io_addr, _: uacpi.uacpi_size, _: [*c]uacpi.uacpi_handle) uacpi.uacpi_status {
    return uacpi.UACPI_STATUS_OK;
}

export fn uacpi_kernel_io_unmap(_: uacpi.uacpi_handle) void {}

export fn uacpi_kernel_io_read(handle: uacpi.uacpi_handle, off: uacpi.uacpi_size, bw: uacpi.uacpi_u8, val: [*c]uacpi.uacpi_u64) uacpi.uacpi_status {
    return uacpi_kernel_raw_io_read(@intFromPtr(handle) + off, bw, val);
}

export fn uacpi_kernel_io_write(handle: uacpi.uacpi_handle, off: uacpi.uacpi_size, bw: uacpi.uacpi_u8, val: uacpi.uacpi_u64) uacpi.uacpi_status {
    return uacpi_kernel_raw_io_write(@intFromPtr(handle) + off, bw, val);
}

export fn uacpi_kernel_pci_read(addr: [*c]uacpi.uacpi_pci_address, off: uacpi.uacpi_size, bw: uacpi.uacpi_u8, val: [*c]uacpi.uacpi_u64) uacpi.uacpi_status {
    _ = addr;
    _ = off;
    _ = bw;
    _ = val;
    return uacpi.UACPI_STATUS_UNIMPLEMENTED;
}

export fn uacpi_kernel_pci_write(addr: [*c]uacpi.uacpi_pci_address, off: uacpi.uacpi_size, bw: uacpi.uacpi_u8, val: uacpi.uacpi_u64) uacpi.uacpi_status {
    _ = addr;
    _ = off;
    _ = bw;
    _ = val;
    return uacpi.UACPI_STATUS_UNIMPLEMENTED;
}

export fn uacpi_kernel_schedule_work(wtype: uacpi.uacpi_work_type, handler: uacpi.uacpi_work_handler, handle: uacpi.uacpi_handle) uacpi.uacpi_status {
    _ = wtype;
    _ = handler;
    _ = handle;
    return uacpi.UACPI_STATUS_UNIMPLEMENTED;
}

export fn uacpi_kernel_wait_for_work_completion() uacpi.uacpi_status {
    return uacpi.UACPI_STATUS_UNIMPLEMENTED;
}

export fn uacpi_kernel_get_thread_id() uacpi.uacpi_thread_id {
    @panic("uacpi_kernel_get_thread_id is unimplemented");
}

export fn uacpi_kernel_alloc(size: uacpi.uacpi_size) ?*anyopaque {
    return @ptrCast((mem.kheap.allocator().alloc(u8, size) catch @panic("OOM")).ptr);
}

export fn uacpi_kernel_calloc(count: uacpi.uacpi_size, size: uacpi.uacpi_size) ?*anyopaque {
    const a = mem.kheap.allocator().alloc(u8, count * size) catch @panic("OOM");
    for (a) |*b| b.* = 0;
    return @ptrCast(a.ptr);
}

export fn uacpi_kernel_free(ptr: ?*anyopaque) void {
    _ = ptr;
    @panic("uacpi_kernel_free is unimplemented");
}

export fn uacpi_kernel_map(addr: uacpi.uacpi_phys_addr, size: uacpi.uacpi_size) ?*anyopaque {
    log.err("uacpi_kernel_map size: {}", .{size});
    const virt = mem.virt(addr);
    mem.kmapper.map(addr, virt, (1 << 63) | 0b11);
    return @ptrFromInt(virt);
}

export fn uacpi_kernel_unmap(addr: ?*anyopaque, size: uacpi.uacpi_size) void {
    log.err("uacpi_kernel_unmap size: {}", .{size});
    _ = addr;
    @panic("uacpi_kernel_unmap unimplemented");
}

export fn uacpi_kernel_get_rsdp(out: [*c]uacpi.uacpi_phys_addr) uacpi.uacpi_status {
    _ = out;
    return uacpi.UACPI_STATUS_UNIMPLEMENTED;
}

export fn uacpi_kernel_get_ticks() uacpi.uacpi_u64 {
    return timers.time();
}

export fn uacpi_kernel_uninstall_interrupt_handler(_: uacpi.uacpi_interrupt_handler, _: uacpi.uacpi_handle) uacpi.uacpi_status {
    return uacpi.UACPI_STATUS_UNIMPLEMENTED;
}

export fn uacpi_kernel_install_interrupt_handler(_: uacpi.uacpi_u32, _: uacpi.uacpi_interrupt_handler, _: uacpi.uacpi_handle, _: [*c]uacpi.uacpi_handle) uacpi.uacpi_status {
    return uacpi.UACPI_STATUS_UNIMPLEMENTED;
}

export fn uacpi_kernel_create_event() uacpi.uacpi_handle {
    @panic("uacpi_kernel_create_event unimplemented");
}

export fn uacpi_kernel_create_spinlock() uacpi.uacpi_handle {
    @panic("uacpi_kernel_create_spinlock unimplemented");
}

export fn uacpi_kernel_lock_spinlock(_: uacpi.uacpi_handle) uacpi.uacpi_cpu_flags {
    @panic("uacpi_kernel_lock_spinlock unimplemented");
}

export fn uacpi_kernel_signal_event(_: uacpi.uacpi_handle) void {
    @panic("uacpi_kernel_signal_event unimplemented");
}

export fn uacpi_kernel_unlock_spinlock(_: uacpi.uacpi_handle, _: uacpi.uacpi_cpu_flags) void {
    @panic("uacpi_kernel_unlock_spinlock unimplemented");
}

export fn uacpi_kernel_sleep(ms: uacpi.uacpi_u64) void {
    timers.sleep(ms);
}

export fn uacpi_kernel_stall(_: uacpi.uacpi_u8) void {
    uacpi_kernel_sleep(1);
}

export fn uacpi_kernel_reset_event(_: uacpi.uacpi_handle) void {
    @panic("uacpi_kernel_reset_event unimplemented");
}

export fn uacpi_kernel_handle_firmware_request(request: [*c]uacpi.uacpi_firmware_request) uacpi.uacpi_status {
    switch (request.*.type) {
        uacpi.UACPI_FIRMWARE_REQUEST_TYPE_BREAKPOINT => log.warn("Ignoring AML breakpoint - CTX: 0x{x}", .{request.*.unnamed_0.breakpoint.ctx orelse 0}),
        uacpi.UACPI_FIRMWARE_REQUEST_TYPE_FATAL => log.err(""),
    }
}
