use core::{alloc::{GlobalAlloc, Layout}, ffi::{c_uchar, c_ulong, c_void}};

use spin::Mutex;
use x86_64::structures::paging::PageTableFlags;

use crate::{mapper::{PTMapper, KMAPPER}, pmm::{self, get_total_size, get_usable_size}, unwrap_mutex, virt_addr};

extern "C" {
    fn buddy_sizeof_alignment(_: c_ulong, _: c_ulong) -> c_ulong;
    fn buddy_init_alignment(_: *mut c_uchar, _: *mut c_uchar, _: c_ulong, _: c_ulong) -> *mut c_void;
    fn buddy_malloc(_: *mut c_void, _: c_ulong) -> *mut c_void;
    fn buddy_safe_free(_: *mut c_void, _: *mut c_void, _: c_ulong);
}

pub struct VirtualMemoryManager<'a> {
    buddy: *mut c_void,
    mpr: &'a Mutex<Option<PTMapper>>,
    flags: PageTableFlags
}

impl<'a> VirtualMemoryManager<'a> {
    pub fn new(mpr: &'a Mutex<Option<PTMapper>>, start: usize, size: u64, flags: PageTableFlags) -> Self {
        let metadata_pages = unsafe { buddy_sizeof_alignment(size, 0x1000) }.div_ceil(0x1000) as usize;
        for i in 0..metadata_pages {
            unwrap_mutex!(mpr).map(pmm::frame(), start + i * 0x1000, flags);
        }
        let at = start as *mut u8;
        Self {
            buddy: unsafe { buddy_init_alignment(at, at.add(metadata_pages * 0x1000), size, 0x1000) },
            mpr,
            flags
        }
    }
}

unsafe impl<'a> GlobalAlloc for VirtualMemoryManager<'a> {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        let size = layout.size() as u64;
        let addr = buddy_malloc(self.buddy, size);
        for i in 0..size.div_ceil(0x1000) {
            unwrap_mutex!(self.mpr).map(pmm::frame(), addr as usize + i as usize * 0x1000, self.flags);
        }
        addr as *mut u8
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
        let size = layout.size();
        buddy_safe_free(self.buddy, ptr as *mut _, size as u64);
        for i in 0..size.div_ceil(0x1000) {
            let ptr = ptr as usize + i * 0x1000;
            pmm::free(mpr.translate(ptr));
            unwrap_mutex!(self.mpr).unmap(ptr);
        }
    }
}

pub(crate) fn init() {
    let vmm = VirtualMemoryManager::new(&KMAPPER, virt_addr(get_total_size() as usize), get_usable_size(), PageTableFlags::WRITABLE | PageTableFlags::NO_EXECUTE);
    log::info!("Initialized virtual memory manager");
}