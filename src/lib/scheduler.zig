const kernel = @import("kernel");
const Process = kernel.lib.Process;
const DoublyLinkedList = @import("std").DoublyLinkedList;
const PriorityQueue = @import("std").PriorityQueue;
const Mutex = kernel.lib.Mutex;
const mem = kernel.lib.mem;
const smp = kernel.drivers.smp;
const cpu = kernel.drivers.cpu;
const lapic = kernel.drivers.lapic;
const idt = kernel.drivers.idt;
const log = @import("std").log.scoped(.sched);

pub const ReadyQueue = DoublyLinkedList(*Process);

// TODO: Separate into kernel and user quantums (50ms, 25 ms)
const QUANTUM = 50;
pub const SCHED_VEC = 0x20;
var idle_proc: Process = undefined;
var ready_queue_start: ?*Process = null;
var ready_queue_end: *Process = undefined;
var lock: Mutex = .{};

pub fn init() void {
    idt.set_ent(SCHED_VEC, idt.create_irq(SCHED_VEC, "sched_handler"));
    const kproc = mem.kheap.allocator().create(Process) catch @panic("OOM");
    kproc.* = .{
        .ef = undefined,
        .mapper = mem.kmapper,
        .next = kproc,
        .status = .running,
    };
    smp.cpu_info(null).cur_proc = kproc;
}

pub fn queue(proc: *Process) void {
    proc.status = .ready;
    if (ready_queue_start == null) {
        ready_queue_start = proc;
        ready_queue_end = proc;
    } else {
        ready_queue_end.next = proc;
        ready_queue_end = proc;
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

export fn sched_handler(status: *const cpu.Status, _: usize) *const cpu.Status {
    const cpu_info = smp.cpu_info(null);
    if (ready_queue_start) |proc| {
        if (cpu_info.cur_proc) |c| {
            c.ef = status.*;
            ready_queue_end.next = c;
            ready_queue_end = c;
        }
        ready_queue_start = proc.next;
        cpu_info.cur_proc = proc;
        cpu.set_cr3(mem.phys(@intFromPtr(proc.mapper.pml4)));
        return &proc.ef;
    } else return status;
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
