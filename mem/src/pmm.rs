use core::mem::transmute;

use limine::{memory_map::EntryType, request::{MemoryMapRequest, PagingModeRequest}};
use spin::Mutex;

use crate::virt_addr;

struct FreeRegion {
    pub start: usize,
    pub num_pages: u64,
    pub next: Option<*mut FreeRegion>
}

pub struct PhysicalMemoryManager {
    free_list: Option<*mut FreeRegion>,
    avail_list: Option<*mut FreeRegion>
}

impl FreeRegion {
    pub fn frame(&mut self) -> (usize, bool) {
        let addr = self.start;
        self.start += 0x1000;
        self.num_pages -= 1;
        (addr, self.num_pages == 0)
    }
}

// We are only accessing PMM through mutex, so I think this should be fine
unsafe impl Send for PhysicalMemoryManager {}

impl PhysicalMemoryManager {
    pub const fn new() -> Self {
        Self { free_list: None, avail_list: None }
    }

    pub fn frame(&mut self) -> usize {
        let region = self.free_list.unwrap();
        let (frame, empty) = unsafe { (*region).frame() };
        if empty {
            self.free_list = unsafe { (*region).next };
            unsafe { (*region).next = self.avail_list };
            self.avail_list = Some(region);
        }
        frame
    }

    pub fn add_region(&mut self, start: usize, num_pages: u64) {
        /*
        If anything in avail_list: use it
        If no regions are added: use start as pointer to region data and reserve frame
        If regions are added:
            Find end of free_list
            If end_region + size of free list overflows to next page:
                alloc frame and use that as start of region data
            else:
                put region data after end_region
         */
        if let Some(region) = self.avail_list {
            unsafe {
                self.avail_list = (*region).next;
                (*region).next = self.free_list;
                (*region).start = start;
                (*region).num_pages = num_pages;
            }
            self.free_list = Some(region);
        } else if let Some(mut region) = self.free_list {
            while let Some(_region) = unsafe { (*region).next } { region = _region; }
            let addr = region as usize + size_of::<FreeRegion>();
            if addr + size_of::<FreeRegion>() >= addr.next_multiple_of(0x1000) {
                // Will overflow, need to allocate new frame
                region = virt_addr(self.frame()) as *mut FreeRegion;
            } else {
                region = addr as *mut FreeRegion;
                log::debug!("New region address: {:#x}", addr);
            }
            unsafe {
                (*region).next = self.free_list;
                (*region).start = start;
                (*region).num_pages = num_pages;
            }
            self.free_list = Some(region);
        } else {
            let region = virt_addr(start) as *mut FreeRegion;
            unsafe {
                (*region).next = None;
                (*region).start = start + 0x1000;
                (*region).num_pages = num_pages - 1;
            }
            self.free_list = Some(region);
        }
    }
}

#[used]
#[link_section = ".requests"]
static MMAP_REQUEST: MemoryMapRequest = MemoryMapRequest::new();

#[used]
#[link_section = ".requests"]
static PAGING_REQUEST: PagingModeRequest = PagingModeRequest::new(); // Already set to default (LVL4)

pub(crate) static PMM: Mutex<PhysicalMemoryManager> = Mutex::new(PhysicalMemoryManager::new());

pub(crate) unsafe fn init() {
    assert!(PAGING_REQUEST.get_response().is_some(), "Limine failed to respond to paging request");

    let mmap_res = MMAP_REQUEST.get_response().unwrap();
    let max_addr: u64 = {
        let mut it = mmap_res.entries().iter().rev();
        let mut entry = it.next();
        while let Some(ent) = entry {
            if ent.entry_type == EntryType::FRAMEBUFFER {
                entry = it.next();
            } else { break; }
        };
        let entry = entry.unwrap();
        entry.base + entry.length
    };
    log::debug!("Found {:#x} bytes of physical memory", max_addr);
    let num_pages = max_addr.div_ceil(0x1000);
    let bitmap_size = num_pages.div_ceil(8);
    let mut bitmap_start = 0;
    for ent in mmap_res.entries() {
        if ent.entry_type == EntryType::USABLE && ent.length >= bitmap_size {
            bitmap_start = ent.base;
            break;
        }
    }
    assert!(bitmap_start != 0, "Could not find area large enough to fit PMM bitmap");
    log::debug!("Bitmap start: {:#x}, size: {:#x}", bitmap_start, bitmap_size);
    // let mut pmm = PhysicalMemoryManager::new(virt_addr(bitmap_start as usize), bitmap_size as usize);
    let mut pmm = PMM.lock();
    for (i, ent) in mmap_res.entries().iter().enumerate() {
        log::debug!("MMAP[{}] - Base: {:#x}, Length: {:#x}, Type: {}", i, ent.base, ent.length, unsafe { transmute::<EntryType, u64>(ent.entry_type) });
        if ent.entry_type == EntryType::USABLE {
            pmm.add_region(ent.base as usize, ent.length / 0x1000);
            // pmm.clear_range(ent.base as usize, ent.length.div_ceil(0x1000) as usize);
        }
    }

    // pmm.set_range(bitmap_start as usize, bitmap_size.div_ceil(0x1000) as usize);
    // let mut p = PMM.lock();
    // *p = Some(pmm);
    log::info!("Initialized physical memory manager");
}
