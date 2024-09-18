use core::alloc::Layout;

use linked_list_allocator::Heap;
use x86_64::structures::paging::PageTableFlags;

use crate::{mapper::KMAPPER, pmm::{MAX_ADDR, PMM}, virt_addr};

pub(crate) fn init() {
    let bottom = *MAX_ADDR.get().unwrap();
    let frm = PMM.lock().frame();
    KMAPPER.lock().as_mut().unwrap().map(frm, virt_addr(bottom), PageTableFlags::WRITABLE | PageTableFlags::NO_EXECUTE);
    // let mut heap = unsafe { Heap::new(virt_addr(bottom) as *mut u8, bottom) };
    // let test = heap.allocate_first_fit(Layout::new::<u8>()).unwrap();
    // log::info!("{:?}", test);
    log::info!("Initialized kernel virtual memory manager");
}