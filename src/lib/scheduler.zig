const kernel = @import("kernel");
const Process = kernel.lib.Process;
const DoublyLinkedList = @import("std").DoublyLinkedList;
const Mutex = kernel.lib.Mutex;
const mem = kernel.lib.mem;
const smp = kernel.drivers.smp;
const cpu = kernel.drivers.cpu;
const lapic = kernel.drivers.lapic;
const idt = kernel.drivers.idt;
const log = @import("std").log.scoped(.sched);

pub const ReadyQueue = DoublyLinkedList(Process);

// TODO: Separate into kernel and user quantums (50ms, 25 ms)
const QUANTUM = 50;
pub const SCHED_VEC = 0x20;
var idle_proc: Process = undefined;
var ready_queue: ReadyQueue = .{};
var lock: Mutex = .{};

pub fn init() void {
    idle_proc = .{
        .heap = null,
        .vmm = null,
        .mapper = &mem.kmapper,
        .ef = .{ .iret_status = .{
            .cs = 0x8,
            .ss = 0x10,
            .rip = @intFromPtr(&idle_thread),
            .rsp = 0,
        } },
    };
    idt.set_ent(SCHED_VEC, idt.create_irq(SCHED_VEC, "sched_handler"));
}

pub fn queue(proc: Process) void {
    const node = mem.kheap.allocator().create(ReadyQueue.Node) catch @panic("OOM");
    node.* = .{ .data = proc };
    lock.lock();
    ready_queue.append(node);
    lock.unlock();
    for (0..smp.SMP_REQUEST.response.?.cpu_count) |i| {
        if (smp.cpu_info(i).cur_proc == null) {
            const id = smp.SMP_REQUEST.response.?.cpus_ptr[i].lapic_id;
            lapic.write_reg(0x310, id << 24);
            lapic.write_reg(0x300, 0x20);
            return;
        }
    }
    // If all threads are running a process, we don't do anything - they will preempt eventually
}

export fn sched_handler(status: *const cpu.Status, _: usize) *const cpu.Status {
    log.info("sched_handler", .{});
    lapic.eoi();
    const cpu_info = smp.cpu_info(null);
    // Check for wakeups

    lock.lock();
    defer lock.unlock();
    if (cpu_info.cur_proc) |c| {
        if (c.data.status == .dead) {
            // Cleanup
            cpu_info.cur_proc = null;
            if (ready_queue.first == null) {
                // Load idle proc
            }
        } else if (c.data.status == .sleep) {
            // Save proc
            // Add to wakeup queue
            cpu_info.cur_proc = null;
            if (ready_queue.first == null) {
                // Load idle proc
            }
        }
    }

    if (ready_queue.popFirst()) |node| {
        if (cpu_info.cur_proc) |c| if (c.data.status == .ready) {
            // Save proc
        };

        cpu.set_cr3(mem.phys(@intFromPtr(node.data.mapper.pml4)));
        lapic.write_reg(lapic.INITIAL_COUNT_OFF, QUANTUM * kernel.drivers.timers.lapic.ms_ticks);
        return &node.data.ef;
    } else return status;
}

fn idle_thread() void {
    while (true) {}
}
