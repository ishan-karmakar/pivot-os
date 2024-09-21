use core::{alloc::{GlobalAlloc, Layout}, ffi::{c_uchar, c_ulong, c_void}};

use limine::memory_map::EntryType;
use x86_64::structures::paging::PageTableFlags;

use crate::{mapper::PTMapper, pmm::{get_mmap, PhysicalMemoryManager}, virt_addr};

extern "C" {
    fn buddy_sizeof_alignment(_: c_ulong, _: c_ulong) -> c_ulong;
    fn buddy_init_alignment(_: *mut c_uchar, _: *mut c_uchar, _: c_ulong, _: c_ulong) -> *mut c_void;
    fn buddy_malloc(_: *mut c_void, _: c_ulong) -> *mut c_void;
    fn buddy_safe_free(_: *mut c_void, _: *mut c_void, _: c_ulong);
}

pub struct VirtualMemoryManager {
    buddy: *mut c_void,
    flags: PageTableFlags
}

impl VirtualMemoryManager {
    pub fn new(start: usize, size: u64, flags: PageTableFlags, mpr: &mut PTMapper, pmm: &mut PhysicalMemoryManager) -> Self {
        let metadata_pages = unsafe { buddy_sizeof_alignment(size, 0x1000) }.div_ceil(0x1000) as usize;
        for i in 0..metadata_pages {
            mpr.map(pmm.frame(), start + i * 0x1000, flags, pmm);
        }
        let at = start as *mut u8;
        Self {
            buddy: unsafe { buddy_init_alignment(at, at.add(metadata_pages * 0x1000), size, 0x1000) },
            mpr,
            flags
        }
    }
}

unsafe impl GlobalAlloc for VirtualMemoryManager {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        let size = layout.size() as u64;
        let addr = buddy_malloc(self.buddy, size);
        for i in 0..size.div_ceil(0x1000) {
            self.mpr.map(PMM.frame(), addr as usize + i as usize * 0x1000, self.flags);
        }
        addr as *mut u8
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
        let size = layout.size();
        buddy_safe_free(self.buddy, ptr as *mut _, size as u64);
        for i in 0..size.div_ceil(0x1000) {
            let ptr = ptr as usize + i * 0x1000;
            PMM.free(self.mpr.translate(ptr));
            self.mpr.unmap(ptr);
        }
    }
}

pub(crate) fn init(mpr: &mut PTMapper, pmm: &mut PhysicalMemoryManager) {
    let mmap = get_mmap();
    let total_size = mmap.iter().map(|m| m.base + m.length).max().unwrap();
    let free_size = mmap.iter().filter(|m| m.entry_type == EntryType::USABLE)
        .map(|m| m.length).sum();
    let vmm = VirtualMemoryManager::new(&KMAPPER, virt_addr(total_size as usize), free_size, PageTableFlags::WRITABLE | PageTableFlags::NO_EXECUTE);
    log::info!("Initialized virtual memory manager");
}