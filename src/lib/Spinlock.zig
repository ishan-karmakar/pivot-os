const atomic = @import("std").atomic;

lock: atomic.Value(bool) = atomic.Value(bool).init(false),

pub fn acquire(self: *@This()) void {
    asm volatile ("cli");
    while (self.lock.swap(true, .acquire))
        atomic.spinLoopHint();
}

pub fn release(self: *@This()) void {
    self.lock.store(false, .release);
    asm volatile ("sti");
}
