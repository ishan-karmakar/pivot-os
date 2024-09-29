use core::mem::transmute;

use limine::{memory_map::{Entry, EntryType}, request::{MemoryMapRequest, PagingModeRequest}};
use spin::{Lazy, Mutex};

use crate::{phys_addr, virt_addr};

struct FreeRegion {
    pub num_pages: usize,
    pub next: Option<*mut FreeRegion>
}

pub struct PhysicalMemoryManager(Option<*mut FreeRegion>);
unsafe impl Send for PhysicalMemoryManager {}

impl FreeRegion {
    pub fn frame(&mut self) -> usize {
        self.num_pages -= 1;
        // We give frames from the end of the region so we don't have to move metadata
        phys_addr(self as *const _ as usize) + self.num_pages * 0x1000
    }
}

// I am using a singly linked list and am only using the head of the list
// Allocation, Freeing, and Insertion of regions are all O(1)
// No extra space is wasted because it is placed at the start of free regions
impl PhysicalMemoryManager {
    pub const fn new() -> Self {
        Self(None)
    }

    // O(1)
    pub fn add_region(&mut self, start: usize, num_pages: usize) {
        let region = virt_addr(start) as *mut FreeRegion;
        unsafe {
            (*region).num_pages = num_pages;
            (*region).next = self.0;
        }
        self.0 = Some(region);
    }

    // O(1)
    pub fn frame(&mut self) -> usize {
        let region = self.0.unwrap();
        let frm = unsafe { (*region).frame() };
        if unsafe { (*region).num_pages } == 0 {
            // Need to remove
            self.0 = unsafe { (*region).next };
        }
        frm
    }

    // O(1)
    pub fn free(&mut self, frm: usize) {
        let region = virt_addr(frm) as *mut FreeRegion;
        unsafe {
            (*region).num_pages = 1;
            (*region).next = self.0;
        }
        self.0 = Some(region);
    }

    // O(n), I think
    // TODO: Implement this, I don't have a need for it right now
    // My brain is too tired to think about this
    // pub fn reserve(&mut self, _: usize) {
        // unimplemented!()
        // let mut region = self.0.unwrap();
        // loop {
        //     let addr = region as usize;
        //     if addr <= frm && frm < (addr + unsafe { (*region).num_pages } * 0x1000) {
        //         break;
        //     } else {
        //         if let Some(r) = unsafe { (*region).next } {
        //             region = r;
        //         } else {
        //             let region = virt_addr(frm) as *mut FreeRegion;
        //             unsafe {
        //                 (*region).num_pages = 1;
        //                 (*region).next = self.0;
        //             }
        //             self.0 = Some(region);
        //             return;
        //         }
        //     }
        // }
    // }
}

#[link_section = ".requests"]
static MMAP_REQUEST: MemoryMapRequest = MemoryMapRequest::new();

#[link_section = ".requests"]
static PAGING_REQUEST: PagingModeRequest = PagingModeRequest::new(); // Already set to default (LVL4)

pub static PMM: Lazy<Mutex<PhysicalMemoryManager>> = Lazy::new(|| {
    assert!(PAGING_REQUEST.get_response().is_some(), "Limine failed to respond to paging request");

    let mmap_res = MMAP_REQUEST.get_response().unwrap();
    let total_size = get_mmap().iter().map(|m| m.base + m.length).max().unwrap();
    let mut pmm = PhysicalMemoryManager::new();
    log::debug!("Found {:#x} bytes of physical memory", total_size);
    for (i, ent) in mmap_res.entries().iter().enumerate() {
        log::debug!("MMAP[{}] - Base: {:#x}, Length: {:#x}, Type: {}", i, ent.base, ent.length, unsafe { transmute::<EntryType, u64>(ent.entry_type) });
        if ent.entry_type == EntryType::USABLE {
            pmm.add_region(ent.base as usize, ent.length as usize / 0x1000);
        }
    }

    log::info!("Initialized physical memory manager");
    Mutex::new(pmm)
});

pub(crate) fn get_mmap() -> &'static [&'static Entry] { MMAP_REQUEST.get_response().unwrap().entries() }
