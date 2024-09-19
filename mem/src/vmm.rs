use core::ffi::{c_uchar, c_void};

use spin::Mutex;
use x86_64::structures::paging::PageTableFlags;

use crate::{mapper::{PTMapper, KMAPPER}, pmm::{get_total_size, get_usable_size, PMM}, unwrap_mutex, virt_addr};

extern "C" {
    fn buddy_sizeof_alignment(_: usize, _: usize) -> usize;
    fn buddy_init_alignment(_: *mut c_uchar, _: *mut c_uchar, _: usize, _: usize) -> *mut c_void;
}

pub struct VirtualMemoryManager<'a> {
    buddy: *const c_void,
    mpr: &Mutex<Option<PTMapper<'a>>>
}

impl<'a> VirtualMemoryManager<'a> {
    pub fn new(mpr: &Mutex<Option<PTMapper>>, start: usize, size: usize, flags: PageTableFlags) -> Self {
        let metadata_pages = unsafe { buddy_sizeof_alignment(size, 0x1000) }.div_ceil(0x1000);
        for i in 0..metadata_pages {
            unwrap_mutex!(mpr).map(PMM.lock().frame(), start + i * 0x1000, flags);
        }
        let at = start as *mut u8;
        Self {
            buddy: unsafe { buddy_init_alignment(at, at.add(metadata_pages * 0x1000), size, 0x1000) },
            mpr
        }
    }
}

pub(crate) fn init() {
    let vmm = VirtualMemoryManager::new(&KMAPPER, virt_addr(get_total_size()), get_usable_size(), PageTableFlags::WRITABLE | PageTableFlags::NO_EXECUTE);
}