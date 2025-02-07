const kernel = @import("kernel");
const idt = kernel.drivers.idt;
const mem = kernel.lib.mem;
const cpu = kernel.drivers.cpu;
const std = @import("std");
const log = std.log.scoped(.sched);

const QUANTUM = 50;

pub var Task = kernel.Task{
    .name = "Scheduler",
    .init = init,
    .dependencies = &.{
        .{ .task = &kernel.lib.mem.KHeapTask },
    },
};

pub const Thread = struct {
    id: usize,
    ef: ?cpu.Status,
    mapper: mem.Mapper,
    stack: ?[]u8,
};

const ThreadLinkedList = std.DoublyLinkedList(Thread);
const ThreadQueue = std.meta.Tuple(&.{ u8, ThreadLinkedList });
const ThreadPriorityQueue = std.PriorityQueue(ThreadQueue, void, compareThreadPriorities);

var sched_vec: *idt.HandlerData = undefined;
var id = std.atomic.Value(usize).init(0);
var lock = std.atomic.Value(bool).init(false);
var global_queue: ThreadPriorityQueue = undefined;

fn init() kernel.Task.Ret {
    global_queue = ThreadPriorityQueue.init(mem.kheap.allocator(), {});

    const kproc = Thread{
        .id = id.fetchAdd(1, .monotonic),
        .ef = null,
        .mapper = mem.kmapper,
        .stack = null,
    };
    enqueue(100, kproc);
    return .success;
}

fn enqueue(priority: u8, thread: Thread) void {
    const node = mem.kheap.allocator().create(ThreadLinkedList.Node) catch @panic("OOM");
    node.data = thread;
    while (lock.cmpxchgWeak(false, true, .acquire, .monotonic) != null) {}

    for (global_queue.items) |*queue| {
        if (queue[0] == priority) {
            queue[1].append(node);
            return;
        }
    }
    var queue = ThreadQueue{ priority, ThreadLinkedList{} };
    queue[1].append(node);
    global_queue.add(queue) catch @panic("OOM");

    lock.store(false, .release);
}

fn compareThreadPriorities(_: void, a: ThreadQueue, b: ThreadQueue) std.math.Order {
    // Highest priority first, lowest priority last
    return std.math.order(b[0], a[0]);
}
