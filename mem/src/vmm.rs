use core::slice;

pub struct VirtualMemoryManager {
    free_lists: *mut &'static [u32]
}

impl VirtualMemoryManager {
    pub fn new(start: usize, size: usize) -> Self {
    }
}

pub(crate) fn init() {}