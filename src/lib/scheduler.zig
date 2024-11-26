const kernel = @import("kernel");
const Process = kernel.lib.Process;
const AutoHashMap = @import("std").AutoHashMap;
const Mutex = kernel.lib.Mutex;
const mem = kernel.lib.mem;
const smp = kernel.drivers.smp;
const cpu = kernel.drivers.cpu;
const lapic = kernel.drivers.lapic;
const idt = kernel.drivers.idt;
const log = @import("std").log.scoped(.sched);
const std = @import("std");

const ReadyQueue = AutoHashMap(u8, *Process);

// TODO: Separate into kernel and user quantums (50ms, 25 ms)
const QUANTUM = 50;
pub const SCHED_VEC = 0x20;
var idle_proc: Process = undefined;
// var ready_queue_start: ?*Process = null;
// var ready_queue_end: *Process = undefined;
var ready_queue = ReadyQueue.init(mem.kheap.allocator());
var lock: Mutex = .{};

pub fn init() void {
    idt.set_ent(SCHED_VEC, idt.create_irq(SCHED_VEC, "sched_handler"));
    const kproc = mem.kheap.allocator().create(Process) catch @panic("OOM");
    kproc.* = .{
        .ef = undefined,
        .mapper = mem.kmapper,
        .next = null,
        .priority = 99,
        .status = .running,
    };
    smp.cpu_info(null).cur_proc = kproc;
}

pub fn queue(proc: *Process) void {
    proc.status = .ready;
    const cpu_info = smp.cpu_info(null);
    lock.lock();
    proc.next = ready_queue.get(proc.priority);
    ready_queue.put(proc.priority, proc) catch @panic("Failed to add to to ready queue");
    lock.unlock();
    if (cpu_info.cur_proc == null or proc.priority > cpu_info.cur_proc.?.priority) {
        asm volatile ("int $0x20");
    }

    // const node = mem.kheap.allocator().create(ReadyQueue.Node) catch @panic("OOM");
    // node.* = .{ .data = proc };
    // lock.lock();
    // ready_queue.append(node);
    // lock.unlock();
    // for (0..smp.SMP_REQUEST.response.?.cpu_count) |i| {
    //     if (smp.cpu_info(i).cur_proc == null) {
    //         const id = smp.SMP_REQUEST.response.?.cpus_ptr[i].lapic_id;
    //         lapic.write_reg(0x310, id << 24);
    //         lapic.write_reg(0x300, 0x20);
    //         break;
    //     }
    // }
    // If all threads are running a process, we don't do anything - they will preempt eventually
}

fn compare_proc(_: void, a: *Process, b: *Process) std.math.Order {
    return std.math.order(a.priority, b.priority);
}

// pub fn sleep(ms: usize) void {
//     const proc = smp.cpu_info(null).cur_proc.?;
//     proc.status = .sleeping;
//     proc.wakeup = ms;
//     if (sleeping_queue_start == null) {
//         sleeping_queue_start = proc;
//         sleeping_queue_end = proc;
//     } else {
//         sleeping_queue_end.next = proc;
//         sleeping_queue_end = proc;
//     }
// }

// pub fn unblock_proc(proc: *Process) void {
//     if (ready_queue_start == null) {
//         ready_queue_start = proc;
//         ready_queue_end = proc;
//         asm volatile ("int $0x20");
//     } else {
//         ready_queue_end.next = proc;
//         ready_queue_end = proc;
//     }
// }

export fn sched_handler(status: *const cpu.Status, _: usize) *const cpu.Status {
    if (@atomicLoad(bool, &lock.locked, .acquire)) return status;
    // log.info("sched_handler", .{});
    log.info("sched_handler", .{});
    lapic.eoi();
    return status;
    // lapic.eoi();
    // const cpu_info = smp.cpu_info(null);
    // // Check for wakeups

    // lock.lock();
    // defer lock.unlock();
    // if (cpu_info.cur_proc) |c| {
    //     if (c.status == .dead) {
    //         // Cleanup
    //         cpu_info.cur_proc = null;
    //         if (ready_queue.first == null) {
    //             // Load idle proc
    //         }
    //     } else if (c.status == .sleep) {
    //         c.ef = status.*;
    //         // Add to wakeup queue
    //         cpu_info.cur_proc = null;
    //         if (ready_queue.first == null) {
    //             // Load idle proc
    //         }
    //     }
    // }

    // if (ready_queue.popFirst()) |node| {
    //     if (cpu_info.cur_proc) |c| {
    //         // Save proc
    //         c.ef = status.*;
    //         const old_node = mem.kheap.allocator().create(ReadyQueue.Node) catch @panic("OOM");
    //         old_node.* = .{ .data = c };
    //         ready_queue.append(old_node);
    //     }
    //     cpu_info.cur_proc = node.data;
    //     mem.kheap.allocator().destroy(node);
    //     cpu.set_cr3(mem.phys(@intFromPtr(cpu_info.cur_proc.?.mapper.pml4)));
    //     lapic.write_reg(lapic.INITIAL_COUNT_OFF, QUANTUM * kernel.drivers.timers.lapic.ms_ticks);
    //     return &node.data.ef;
    // } else if (cpu_info.cur_proc != null) {
    //     lapic.write_reg(lapic.INITIAL_COUNT_OFF, QUANTUM * kernel.drivers.timers.lapic.ms_ticks);
    // }
}

fn idle_thread() void {
    while (true) {}
}
