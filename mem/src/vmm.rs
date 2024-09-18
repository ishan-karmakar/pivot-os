use x86_64::structures::paging::PageTableFlags;

use crate::{mapper::KMAPPER, pmm::{MAX_ADDR, PMM}, virt_addr};

struct Block {
    pub size: usize, // Always a power of two
    pub sibling: Option<*mut Block>
}

pub struct VirtualMemoryManager<const MB: usize = 0x1000>(*mut Block);

impl<const MB: usize> VirtualMemoryManager<MB> {
    pub fn new(mut start: usize, mut size: usize) -> Self {
        size = (size / MB) * MB;
        let root_block = start as *mut Block;
        let mut cur_block = root_block;
        loop {
            let block_size = 2_usize.pow(size.ilog2());
            size -= block_size as usize;
            KMAPPER.lock().as_mut().unwrap().map(PMM.lock().frame(), start, PageTableFlags::NO_EXECUTE | PageTableFlags::WRITABLE);
            unsafe { (*cur_block).size = block_size };
            if size == 0 {
                unsafe { (*cur_block).sibling = None };
                return Self(root_block);
            } else {
                start += block_size;
                unsafe { (*cur_block).sibling = Some(start as *mut _); }
                cur_block = start as *mut _;
            }
        }
    }
}

pub(crate) fn init() {
    let bottom = *MAX_ADDR.get().unwrap();
    let vmm: VirtualMemoryManager<> = VirtualMemoryManager::new(virt_addr(bottom), bottom);
    log::info!("Initialized kernel virtual memory manager");
}