const kernel = @import("kernel");
const uacpi = @import("uacpi");
const serial = kernel.drivers.serial;
const mem = kernel.lib.mem;
const idt = kernel.drivers.idt;
const cpu = kernel.drivers.cpu;
const math = @import("std").math;
const log = @import("std").log.scoped(.uacpi);
const Mutex = kernel.lib.Mutex;
const limine = @import("limine");
const std = @import("std");

const UACPI_CTX = std.meta.Tuple(&.{ uacpi.uacpi_interrupt_handler, uacpi.uacpi_handle });

export var RSDP_REQUEST: limine.RsdpRequest = .{ .revision = 3 };

const HandlerInfo = struct {
    ctx: uacpi.uacpi_handle,
    handler: uacpi.uacpi_interrupt_handler,
    vec: u8,
};

export fn uacpi_kernel_raw_memory_read(addr: uacpi.uacpi_phys_addr, bw: uacpi.uacpi_u8, out: [*c]uacpi.uacpi_u64) uacpi.uacpi_status {
    out.* = switch (bw) {
        1 => @intCast(@as(*const u8, @ptrFromInt(addr)).*),
        2 => @intCast(@as(*const u16, @ptrFromInt(addr)).*),
        4 => @intCast(@as(*const u32, @ptrFromInt(addr)).*),
        8 => @as(*const u64, @ptrFromInt(addr)).*,
        else => return uacpi.UACPI_STATUS_TYPE_MISMATCH,
    };
    @panic("uacpi_kernel_raw_memory_read is unimplemented");
}

export fn uacpi_kernel_raw_memory_write(addr: uacpi.uacpi_phys_addr, bw: uacpi.uacpi_u8, val: uacpi.uacpi_u64) uacpi.uacpi_status {
    switch (bw) {
        1 => @as(*u8, @ptrFromInt(addr)).* = @truncate(val),
        2 => @as(*u16, @ptrFromInt(addr)).* = @truncate(val),
        4 => @as(*u32, @ptrFromInt(addr)).* = @truncate(val),
        8 => @as(*u64, @ptrFromInt(addr)).* = val,
        else => return uacpi.UACPI_STATUS_TYPE_MISMATCH,
    }
    @panic("uacpi_kernel_raw_memory_write is unimplemented");
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

export fn uacpi_kernel_io_map(addr: uacpi.uacpi_io_addr, _: uacpi.uacpi_size, out: [*c]uacpi.uacpi_handle) uacpi.uacpi_status {
    out.* = @ptrFromInt(addr);
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
    @panic("uacpi_kernel_pci_read");
    // if (addr.*.segment != 0) @panic("uacpi_kernel_pci_read segment != 0");
    // val.* = switch (bw) {
    //     1 => @intCast(pci.read(addr.*.bus, addr.*.device, addr.*.function, @intCast(off), u8)),
    //     2 => @intCast(pci.read(addr.*.bus, addr.*.device, addr.*.function, @intCast(off), u16)),
    //     4 => @intCast(pci.read(addr.*.bus, addr.*.device, addr.*.function, @intCast(off), u32)),
    //     else => @panic("uacpi_kernel_pci_read received invalid byte width"),
    // };
    // return uacpi.UACPI_STATUS_OK;
}

export fn uacpi_kernel_pci_write(addr: [*c]uacpi.uacpi_pci_address, off: uacpi.uacpi_size, bw: uacpi.uacpi_u8, val: uacpi.uacpi_u64) uacpi.uacpi_status {
    _ = addr;
    _ = off;
    _ = bw;
    _ = val;
    @panic("uacpi_kernel_pci_write");
    // if (addr.*.segment != 0) @panic("uacpi_kernel_pci_write segment != 0");
    // switch (bw) {
    //     1 => pci.write(addr.*.bus, addr.*.device, addr.*.function, @intCast(off), @as(u8, @intCast(val))),
    //     2 => pci.write(addr.*.bus, addr.*.device, addr.*.function, @intCast(off), @as(u16, @intCast(val))),
    //     4 => pci.write(addr.*.bus, addr.*.device, addr.*.function, @intCast(off), @as(u32, @intCast(val))),
    //     else => @panic("uacpi_kernel_pci_write received invalid byte width"),
    // }
    // return uacpi.UACPI_STATUS_OK;
}

export fn uacpi_kernel_schedule_work(wtype: uacpi.uacpi_work_type, handler: uacpi.uacpi_work_handler, handle: uacpi.uacpi_handle) uacpi.uacpi_status {
    _ = wtype;
    _ = handler;
    _ = handle;
    @panic("uacpi_kernel_schedule_work is unimplemented");
    // return uacpi.UACPI_STATUS_UNIMPLEMENTED;
}

export fn uacpi_kernel_wait_for_work_completion() uacpi.uacpi_status {
    @panic("uacpi_kernel_wait_for_work_completion is unimplemented");
    // return uacpi.UACPI_STATUS_UNIMPLEMENTED;
}

export fn uacpi_kernel_get_thread_id() uacpi.uacpi_thread_id {
    return null; // 0
}

export fn uacpi_kernel_alloc(size: uacpi.uacpi_size) ?*anyopaque {
    const ptr = mem.kheap.allocator().alignedAlloc(u8, 8, size) catch @panic("OOM");
    return @ptrCast(ptr.ptr);
}

export fn uacpi_kernel_calloc(count: uacpi.uacpi_size, size: uacpi.uacpi_size) ?*anyopaque {
    // We are just using a alignment of 8 here as a constant. Ideally, we should test what the minimum needed is.
    const a = mem.kheap.allocator().alignedAlloc(u8, 8, count * size) catch @panic("OOM");
    for (a) |*b| b.* = 0;
    return @ptrCast(a.ptr);
}

export fn uacpi_kernel_free(_ptr: ?*anyopaque, size: uacpi.uacpi_size) void {
    const ptr: [*]const u8 = @ptrCast(_ptr orelse return);
    mem.kheap.allocator().free(ptr[0..size]);
}

export fn uacpi_kernel_map(_addr: uacpi.uacpi_phys_addr, size: uacpi.uacpi_size) ?*anyopaque {
    var start = @divFloor(_addr, 0x1000) * 0x1000;
    const end = @divFloor(_addr + size, 0x1000) * 0x1000;
    while (start <= end) {
        mem.kmapper.map(start, start, 0b10);
        start += 0x1000;
    }
    return @ptrFromInt(_addr);
}

export fn uacpi_kernel_unmap(_: ?*anyopaque, _: uacpi.uacpi_size) void {}

export fn uacpi_kernel_get_rsdp(out: [*c]uacpi.uacpi_phys_addr) uacpi.uacpi_status {
    const req = RSDP_REQUEST.response orelse @panic("Limine RSDP request is null");
    out.* = @intFromPtr(req.address);
    return uacpi.UACPI_STATUS_OK;
}

export fn uacpi_kernel_get_ticks() uacpi.uacpi_u64 {
    @panic("uacpi_kernel_get_ticks unimplemented");
}

export fn uacpi_kernel_uninstall_interrupt_handler(_: uacpi.uacpi_interrupt_handler, _: uacpi.uacpi_handle) uacpi.uacpi_status {
    @panic("uacpi_kernel_uninstall_interrupt_handler unimplemented");
    // return uacpi.UACPI_STATUS_UNIMPLEMENTED;
}

export fn uacpi_kernel_install_interrupt_handler(irq: uacpi.uacpi_u32, handler: uacpi.uacpi_interrupt_handler, ctx: uacpi.uacpi_handle, _: [*c]uacpi.uacpi_handle) uacpi.uacpi_status {
    _ = irq;
    _ = handler;
    _ = ctx;
    @panic("uacpi_kernel_install_interrupt_handler");
    // const info = mem.kheap.allocator().create(HandlerInfo) catch @panic("OOM");
    // info.ctx = ctx;
    // info.handler = handler;
    // info.vec = idt.allocate_vec(@truncate(irq + 0x20), uacpi_irq_handler, info) orelse @panic("Out of interrupt handlers");
    // intctrl.set(info.vec, @truncate(irq), 0);
    // intctrl.mask(@truncate(irq), false);
    // @panic("Ran out of interrupt handlers for uACPI");
}

fn uacpi_irq_handler(ctx: ?*anyopaque, status: *const cpu.Status) *const cpu.Status {
    const info: *HandlerInfo = @alignCast(@ptrCast(ctx));
    if (info.handler.?(info.ctx) != uacpi.UACPI_INTERRUPT_HANDLED) @panic("uacpi interrupt not handled");
    return status;
}

export fn uacpi_kernel_create_event() uacpi.uacpi_handle {
    return @ptrCast(mem.kheap.allocator().create(bool) catch @panic("OOM"));
}

export fn uacpi_kernel_create_spinlock() uacpi.uacpi_handle {
    return uacpi_kernel_create_mutex();
}

export fn uacpi_kernel_lock_spinlock(handle: uacpi.uacpi_handle) uacpi.uacpi_cpu_flags {
    asm volatile ("cli");
    _ = uacpi_kernel_acquire_mutex(handle, 0xFFFF);
    return 0;
}

export fn uacpi_kernel_signal_event(_: uacpi.uacpi_handle) void {
    @panic("uacpi_kernel_signal_event unimplemented");
}

export fn uacpi_kernel_unlock_spinlock(handle: uacpi.uacpi_handle, _: uacpi.uacpi_cpu_flags) void {
    uacpi_kernel_release_mutex(handle);
    asm volatile ("sti");
}

export fn uacpi_kernel_sleep(ms: uacpi.uacpi_u64) void {
    _ = ms;
    @panic("uacpi_kernel_sleep unimplemented");
    // timers.sleep(ms);
}

export fn uacpi_kernel_stall(_: uacpi.uacpi_u8) void {
    uacpi_kernel_sleep(1);
}

export fn uacpi_kernel_reset_event(_: uacpi.uacpi_handle) void {
    @panic("uacpi_kernel_reset_event unimplemented");
}

export fn uacpi_kernel_handle_firmware_request(request: [*c]uacpi.uacpi_firmware_request) uacpi.uacpi_status {
    switch (request.*.type) {
        uacpi.UACPI_FIRMWARE_REQUEST_TYPE_BREAKPOINT => log.warn("Ignoring AML breakpoint - CTX: 0x{?}", .{request.*.unnamed_0.breakpoint.ctx}),
        uacpi.UACPI_FIRMWARE_REQUEST_TYPE_FATAL => log.err("Fatal firmware error - type: {}, code: {}, arg: {}", .{
            request.*.unnamed_0.fatal.type,
            request.*.unnamed_0.fatal.code,
            request.*.unnamed_0.fatal.arg,
        }),
        else => return uacpi.UACPI_STATUS_TYPE_MISMATCH,
    }
    return uacpi.UACPI_STATUS_OK;
}

export fn uacpi_kernel_wait_for_event(_: uacpi.uacpi_handle, _: uacpi.uacpi_u16) uacpi.uacpi_bool {
    @panic("uacpi_kernel_wait_for_event unimplemented");
}

// TODO: Handle timeout
export fn uacpi_kernel_acquire_mutex(handle: uacpi.uacpi_handle, _: uacpi.uacpi_u16) uacpi.uacpi_bool {
    const mutex: *Mutex = @ptrCast(handle);
    mutex.lock();
    return true;
}

export fn uacpi_kernel_create_mutex() uacpi.uacpi_handle {
    const mutex = mem.kheap.allocator().create(Mutex) catch @panic("OOM");
    mutex.* = Mutex{};
    return @ptrCast(mutex);
}

export fn uacpi_kernel_free_mutex(_: uacpi.uacpi_handle) void {
    @panic("uacpi_kernel_free_mutex unimplemented");
}

export fn uacpi_kernel_release_mutex(handle: uacpi.uacpi_handle) void {
    const mutex: *Mutex = @ptrCast(handle);
    mutex.unlock();
}

export fn uacpi_kernel_log(level: uacpi.uacpi_log_level, msg: [*c]const uacpi.uacpi_char) void {
    var str: []const u8 = std.mem.span(msg);
    str = str[0 .. str.len - 1];
    switch (level) {
        uacpi.UACPI_LOG_DEBUG, uacpi.UACPI_LOG_TRACE => log.debug("{s}", .{str}),
        uacpi.UACPI_LOG_INFO => log.info("{s}", .{str}),
        uacpi.UACPI_LOG_WARN => log.warn("{s}", .{str}),
        uacpi.UACPI_LOG_ERROR => log.err("{s}", .{str}),
        else => @panic("uacpi_kernel_log received invalid log level"),
    }
}

export fn uacpi_kernel_free_spinlock(_: uacpi.uacpi_handle) void {
    @panic("uacpi_kernel_free_spinlock unimplemented");
}

export fn uacpi_kernel_free_event(_: uacpi.uacpi_handle) void {
    @panic("uacpi_kernel_free_event unimplemented");
}
