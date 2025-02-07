const kernel = @import("kernel");
const uacpi = @import("uacpi");
const serial = kernel.drivers.serial;
const mem = kernel.lib.mem;
const idt = kernel.drivers.idt;
const cpu = kernel.drivers.cpu;
const math = @import("std").math;
const log = @import("std").log.scoped(.uacpi);
const limine = @import("limine");
const std = @import("std");

const UACPI_CTX = std.meta.Tuple(&.{ uacpi.uacpi_interrupt_handler, uacpi.uacpi_handle });

export var RSDP_REQUEST: limine.RsdpRequest = .{ .revision = 3 };

const HandlerInfo = struct {
    ctx: uacpi.uacpi_handle,
    handler: uacpi.uacpi_interrupt_handler,
    vec: u8,
};

export fn uacpi_kernel_io_map(addr: uacpi.uacpi_io_addr, _: uacpi.uacpi_size, out: [*c]uacpi.uacpi_handle) uacpi.uacpi_status {
    out.* = @ptrFromInt(addr);
    return uacpi.UACPI_STATUS_OK;
}

export fn uacpi_kernel_io_unmap(_: uacpi.uacpi_handle) void {}

export fn uacpi_kernel_io_read8(handle: uacpi.uacpi_handle, off: uacpi.uacpi_size, out: [*c]uacpi.uacpi_u8) uacpi.uacpi_status {
    out.* = serial.in(@intCast(@intFromPtr(handle.?) + off), u8);
    return uacpi.UACPI_STATUS_OK;
}

export fn uacpi_kernel_io_read16(handle: uacpi.uacpi_handle, off: uacpi.uacpi_size, out: [*c]uacpi.uacpi_u16) uacpi.uacpi_status {
    out.* = serial.in(@intCast(@intFromPtr(handle.?) + off), u16);
    return uacpi.UACPI_STATUS_OK;
}

export fn uacpi_kernel_io_read32(handle: uacpi.uacpi_handle, off: uacpi.uacpi_size, out: [*c]uacpi.uacpi_u32) uacpi.uacpi_status {
    out.* = serial.in(@intCast(@intFromPtr(handle.?) + off), u32);
    return uacpi.UACPI_STATUS_OK;
}

export fn uacpi_kernel_io_write8(handle: uacpi.uacpi_handle, off: uacpi.uacpi_size, val: uacpi.uacpi_u8) uacpi.uacpi_status {
    serial.out(@intCast(@intFromPtr(handle.?) + off), val);
    return uacpi.UACPI_STATUS_OK;
}

export fn uacpi_kernel_io_write16(handle: uacpi.uacpi_handle, off: uacpi.uacpi_size, val: uacpi.uacpi_u16) uacpi.uacpi_status {
    serial.out(@intCast(@intFromPtr(handle.?) + off), val);
    return uacpi.UACPI_STATUS_OK;
}

export fn uacpi_kernel_io_write32(handle: uacpi.uacpi_handle, off: uacpi.uacpi_size, val: uacpi.uacpi_u32) uacpi.uacpi_status {
    serial.out(@intCast(@intFromPtr(handle.?) + off), val);
    return uacpi.UACPI_STATUS_OK;
}

export fn uacpi_kernel_pci_device_open(addr: uacpi.uacpi_pci_address, out: [*c]uacpi.uacpi_handle) uacpi.uacpi_status {
    const alloced_addr = mem.kheap.allocator().create(uacpi.uacpi_pci_address) catch return uacpi.UACPI_STATUS_OUT_OF_MEMORY;
    alloced_addr.* = addr;
    out.* = alloced_addr;
    return uacpi.UACPI_STATUS_OK;
}

export fn uacpi_kernel_pci_device_close(handle: uacpi.uacpi_handle) void {
    const addr: *uacpi.uacpi_pci_address = @alignCast(@ptrCast(handle));
    mem.kheap.allocator().destroy(addr);
}

export fn uacpi_kernel_pci_read8(handle: uacpi.uacpi_handle, off: uacpi.uacpi_size, val: [*c]uacpi.uacpi_u8) uacpi.uacpi_status {
    _ = handle;
    _ = off;
    _ = val;
    @panic("uacpi_kernel_pci_read8");
}

export fn uacpi_kernel_pci_read16(handle: uacpi.uacpi_handle, off: uacpi.uacpi_size, val: [*c]uacpi.uacpi_u16) uacpi.uacpi_status {
    _ = handle;
    _ = off;
    _ = val;
    @panic("uacpi_kernel_pci_read16");
}

export fn uacpi_kernel_pci_read32(handle: uacpi.uacpi_handle, off: uacpi.uacpi_size, val: [*c]uacpi.uacpi_u32) uacpi.uacpi_status {
    _ = handle;
    _ = off;
    _ = val;
    @panic("uacpi_kernel_pci_read32");
}

export fn uacpi_kernel_pci_write8(handle: uacpi.uacpi_handle, off: uacpi.uacpi_size, val: uacpi.uacpi_u8) uacpi.uacpi_status {
    _ = handle;
    _ = off;
    _ = val;
    @panic("uacpi_kernel_write8");
}

export fn uacpi_kernel_pci_write16(handle: uacpi.uacpi_handle, off: uacpi.uacpi_size, val: uacpi.uacpi_u16) uacpi.uacpi_status {
    _ = handle;
    _ = off;
    _ = val;
    @panic("uacpi_kernel_write16");
}

export fn uacpi_kernel_pci_write32(handle: uacpi.uacpi_handle, off: uacpi.uacpi_size, val: uacpi.uacpi_u32) uacpi.uacpi_status {
    _ = handle;
    _ = off;
    _ = val;
    @panic("uacpi_kernel_write32");
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

export fn uacpi_kernel_free(_ptr: ?*anyopaque, size: uacpi.uacpi_size) void {
    const ptr: [*]const u8 = @ptrCast(_ptr orelse return);
    mem.kheap.allocator().free(ptr[0..size]);
}

export fn uacpi_kernel_map(_addr: uacpi.uacpi_phys_addr, size: uacpi.uacpi_size) ?*anyopaque {
    var start = @divFloor(_addr, 0x1000) * 0x1000;
    const end = @divFloor(_addr + size, 0x1000) * 0x1000;
    while (start <= end) {
        mem.kmapper.map(start, start, 0b11);
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

export fn uacpi_kernel_get_nanoseconds_since_boot() uacpi.uacpi_u64 {
    @panic("uacpi_kernel_get_nanoseconds_since_boot");
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

export fn uacpi_kernel_acquire_mutex(handle: uacpi.uacpi_handle, _: uacpi.uacpi_u16) uacpi.uacpi_bool {
    _ = handle;
    // This function is run before any timers are initialized, so there is no point in handling timeout
    @panic("uacpi_kernel_acquire_mutex");
    // const mutex: *Mutex = @ptrCast(handle);
    // mutex.lock();
    // return true;
}

export fn uacpi_kernel_create_mutex() uacpi.uacpi_handle {
    @panic("uacpi_kernel_create_mutex");
    // const mutex = mem.kheap.allocator().create(std.atomic.Value(bool)) catch @panic("OOM");
    // mutex.* = Mutex{};
    // return @ptrCast(mutex);
}

export fn uacpi_kernel_free_mutex(_: uacpi.uacpi_handle) void {
    @panic("uacpi_kernel_free_mutex unimplemented");
}

export fn uacpi_kernel_release_mutex(handle: uacpi.uacpi_handle) void {
    _ = handle;
    @panic("uacpi_kernel_release_mutex");
    // const mutex: *Mutex = @ptrCast(handle);
    // mutex.unlock();
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
