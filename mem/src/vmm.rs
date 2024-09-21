use core::{alloc::{GlobalAlloc, Layout}, ffi::{c_uchar, c_ulong, c_void}};

use limine::memory_map::EntryType;
use spin::Mutex;
use x86_64::structures::paging::PageTableFlags;

use crate::{mapper::PTMapper, pmm::{get_mmap, PhysicalMemoryManager}, virt_addr};

extern "C" {
    fn buddy_sizeof_alignment(_: c_ulong, _: c_ulong) -> c_ulong;
    fn buddy_init_alignment(_: *mut c_uchar, _: *mut c_uchar, _: c_ulong, _: c_ulong) -> *mut c_void;
    fn buddy_malloc(_: *mut c_void, _: c_ulong) -> *mut c_void;
    fn buddy_safe_free(_: *mut c_void, _: *mut c_void, _: c_ulong);
}

pub struct VirtualMemoryManager<'a, 'b> {
    buddy: *mut c_void,
    mpr: &'a Mutex<PTMapper<'b>>,
    pmm: &'a Mutex<PhysicalMemoryManager>,
    flags: PageTableFlags
}

impl<'a, 'b> VirtualMemoryManager<'a, 'b> {
    pub fn new(start: usize, size: u64, flags: PageTableFlags, mpr: &'a Mutex<PTMapper<'b>>, pmm: &'a Mutex<PhysicalMemoryManager>) -> Self {
        let metadata_pages = unsafe { buddy_sizeof_alignment(size, 0x1000) }.div_ceil(0x1000) as usize;
        for i in 0..metadata_pages {
            let frm = pmm.lock().frame();
            mpr.lock().map(frm, start + i * 0x1000, flags);
        }
        let at = start as *mut u8;
        Self {
            buddy: unsafe { buddy_init_alignment(at, at.add(metadata_pages * 0x1000), size, 0x1000) },
            mpr,
            pmm,
            flags
        }
    }
}

unsafe impl GlobalAlloc for VirtualMemoryManager<'_, '_> {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        let size = layout.size() as u64;
        let addr = unsafe { buddy_malloc(self.buddy, size) };
        for i in 0..size.div_ceil(0x1000) {
            self.mpr.lock().map(self.pmm.lock().frame(), addr as usize + i as usize * 0x1000, self.flags);
        }
        addr as *mut u8
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
        let size = layout.size();
        buddy_safe_free(self.buddy, ptr as *mut _, size as u64);
        for i in 0..size.div_ceil(0x1000) {
            let ptr = ptr as usize + i * 0x1000;
            self.pmm.lock().free(self.mpr.lock().translate(ptr));
            self.mpr.lock().unmap(ptr);
        }
    }
}

pub fn init<'a, 'b>(mpr: &'a Mutex<PTMapper<'b>>, pmm: &'a Mutex<PhysicalMemoryManager>) -> Mutex<VirtualMemoryManager<'a, 'b>> {
    let mmap = get_mmap();
    let total_size = mmap.iter().map(|m| m.base + m.length).max().unwrap();
    let free_size = mmap.iter().filter(|m| m.entry_type == EntryType::USABLE)
        .map(|m| m.length).sum();
    let vmm = VirtualMemoryManager::new(virt_addr(total_size as usize), free_size, PageTableFlags::WRITABLE | PageTableFlags::NO_EXECUTE, mpr, pmm);
    log::info!("Initialized virtual memory manager");
    Mutex::new(vmm)
}
