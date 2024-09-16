use core::{cmp::min, mem::transmute};

use limine::{memory_map::EntryType, request::{MemoryMapRequest, PagingModeRequest}};
use spin::Mutex;

use crate::virt_addr;

pub struct PhysicalMemoryManager {
    bitmap: *mut u8,
    size: usize,
    ffa: usize // First Free Area (optimization that stores first area that (could be) free)
}

impl PhysicalMemoryManager {
    pub fn new(start: usize, size: usize) -> Self {
        let pmm = Self {
            bitmap: start as *mut u8,
            size,
            ffa: 0
        };
        unsafe { pmm.bitmap.write_bytes(0xFF, size) };
        pmm
    }

    pub fn set(&mut self, mut addr: usize) {
        addr /= 0x1000;
        if addr >= self.size * 8 { return; }
        unsafe { *self.bitmap.add(addr / 8) |= 1u8 << (addr % 8) };
    }

    pub fn set_range(&mut self, addr: usize, num_pages: usize) {
        for i in 0..num_pages {
            self.set(addr + i * 0x1000);
        }
    }

    pub fn clear(&mut self, mut addr: usize) {
        addr /= 0x1000;
        if addr >= self.size * 8 { return; }
        unsafe { *self.bitmap.add(addr / 8) &= !(1 << (addr % 8)) };
        self.ffa = min(self.ffa, addr);
    }

    pub fn clear_range(&mut self, addr: usize, num_pages: usize) {
        for i in 0..num_pages {
            self.clear(addr + i * 0x1000);
        }
    }

    pub fn frame(&mut self) -> usize {
        let mut off = self.ffa;
        while off < self.size * 8 {
            let row = off / 8;
            let col = off % 8;
            let row_data = unsafe { *self.bitmap.add(row) };
            if row_data == 0xFF {
                off += 8;
                continue;
            }
            if row_data & (1 << col) == 0 {
                self.ffa = off + 1;
                // Does the equivalent of set() but we already have most of data
                unsafe { *self.bitmap.add(row) = row_data | (1 << col) };
                return off * 0x1000;
            }
            off += 1;
        }
        log::warn!("Could not find a free frame in bitmap");
        return 0;
    }
}

unsafe impl Send for PhysicalMemoryManager {}

// TODO: Add the actual limine requests here instead of using extern
extern "Rust" {
    static MMAP_REQUEST: MemoryMapRequest;
    static PAGING_REQUEST: PagingModeRequest;
}

pub(crate) static PMM: Mutex<Option<PhysicalMemoryManager>> = Mutex::new(None);

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
    let mut pmm = PhysicalMemoryManager::new(virt_addr(bitmap_start as usize), bitmap_size as usize);
    for (i, ent) in mmap_res.entries().iter().enumerate() {
        log::debug!("MMAP[{}] - Base: {:#x}, Length: {:#x}, Type: {}", i, ent.base, ent.length, unsafe { transmute::<EntryType, u64>(ent.entry_type) });
        if ent.entry_type == EntryType::USABLE {
            pmm.clear_range(ent.base as usize, ent.length.div_ceil(0x1000) as usize);
        }
    }

    pmm.set_range(bitmap_start as usize, bitmap_size.div_ceil(0x1000) as usize);
    let mut p = PMM.lock();
    *p = Some(pmm);
    log::info!("Initialized physical memory manager");
}
