locked: bool = false,

pub inline fn lock(self: *@This()) void {
    while (@cmpxchgWeak(bool, &self.locked, false, true, .acquire, .monotonic) != null) {}
}

pub inline fn unlock(self: *@This()) void {
    @atomicStore(bool, &self.locked, false, .release);
}
