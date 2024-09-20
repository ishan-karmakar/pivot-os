use core::mem::transmute;

use limine::{memory_map::EntryType, request::{MemoryMapRequest, PagingModeRequest}};
use spin::{Mutex, Once};

use crate::{phys_addr, virt_addr};

struct FreeRegion {
    pub num_pages: usize,
    pub next: Option<*mut FreeRegion>
}

pub struct PhysicalMemoryManager {
    free_list: Option<*mut FreeRegion>
}

impl FreeRegion {
    pub fn frame(&mut self) -> usize {
        self.num_pages -= 1;
        // We give frames from the end of the region so we don't have to move metadata
        phys_addr(self as *const _ as usize) + self.num_pages * 0x1000
    }
}

// We are only accessing PMM through a mutex
unsafe impl Send for PhysicalMemoryManager {}

// I am using a singly linked list and am only using the head of the list
// Allocation, Freeing, and Insertion of regions are all O(1)
// No extra space is wasted because it is placed at the start of free regions
impl PhysicalMemoryManager {
    pub const fn new() -> Self {
        Self { free_list: None }
    }

    // O(1)
    pub fn add_region(&mut self, start: usize, num_pages: usize) {
        let region = virt_addr(start) as *mut FreeRegion;
        unsafe {
            (*region).num_pages = num_pages;
            (*region).next = self.free_list;
        }
        self.free_list = Some(region);
    }

    // O(1)
    pub fn frame(&mut self) -> usize {
        let region = self.free_list.unwrap();
        let frm = unsafe { (*region).frame() };
        if unsafe { (*region).num_pages } == 0 {
            // Need to remove
            self.free_list = unsafe { (*region).next };
        }
        frm
    }

    // O(1)
    pub fn free(&mut self, frm: usize) {
        let region = virt_addr(frm) as *mut FreeRegion;
        unsafe {
            (*region).num_pages = 1;
            (*region).next = self.free_list;
        }
        self.free_list = Some(region);
    }
}

#[used]
#[link_section = ".requests"]
static MMAP_REQUEST: MemoryMapRequest = MemoryMapRequest::new();

#[used]
#[link_section = ".requests"]
static PAGING_REQUEST: PagingModeRequest = PagingModeRequest::new(); // Already set to default (LVL4)

static PMM: Mutex<PhysicalMemoryManager> = Mutex::new(PhysicalMemoryManager::new());
static MEM_SIZE: Once<u64> = Once::new();
static USABLE_SIZE: Once<u64> = Once::new();

#[inline]
pub(crate) fn get_total_size() -> u64 { *MEM_SIZE.get().unwrap() }

#[inline]
pub(crate) fn get_usable_size() -> u64 { *USABLE_SIZE.get().unwrap() }

#[inline]
// We can't expose PMM static because PMM.lock needs to be dropped
pub(crate) fn frame() -> usize { return PMM.lock().frame(); }

pub(crate) unsafe fn init() {
    assert!(PAGING_REQUEST.get_response().is_some(), "Limine failed to respond to paging request");

    let mmap_res = MMAP_REQUEST.get_response().unwrap();
    MEM_SIZE.call_once(|| mmap_res.entries().last().map(|f| f.base + f.length).unwrap());
    USABLE_SIZE.call_once(|| mmap_res.entries().iter().filter(|m| m.entry_type == EntryType::USABLE).map(|m| m.length).sum::<u64>());
    log::debug!("Found {:#x} bytes of physical memory", get_total_size());
    let mut pmm = PMM.lock();
    for (i, ent) in mmap_res.entries().iter().enumerate() {
        log::debug!("MMAP[{}] - Base: {:#x}, Length: {:#x}, Type: {}", i, ent.base, ent.length, unsafe { transmute::<EntryType, u64>(ent.entry_type) });
        if ent.entry_type == EntryType::USABLE {
            pmm.add_region(ent.base as usize, ent.length as usize / 0x1000);
        }
    }

    log::info!("Initialized physical memory manager");
}
