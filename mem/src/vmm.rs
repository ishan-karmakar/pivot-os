use core::ptr::{slice_from_raw_parts_mut, NonNull};

use bitflags::bitflags;
use limine::memory_map::EntryType;
use spin::{Lazy, Mutex};
use x86_64::structures::paging::PageTableFlags;

use crate::{mapper::{PTMapper, KMAPPER}, pmm::{get_mmap, PMM}, virt_addr};

bitflags! {
    struct BuddyStatus: u8 {
        const USED = 1 << 0;
        const INTERNAL = 1 << 1;
    }
}

struct BuddyBitmap<const MIN_BSIZE: usize> {
    bitmap: &'static mut [u8],
    internal_blocks: usize
}

pub struct VirtualMemoryManager<'a> {
    max_bsize: usize,
    start: usize,
    mpr: &'a Mutex<PTMapper>,
    flags: PageTableFlags,
    bitmap: BuddyBitmap::<0x1000>
}

impl<'a> VirtualMemoryManager<'a> {
    pub fn new(start: usize, size: usize, flags: PageTableFlags, mpr: &'a Mutex<PTMapper>) -> Self {
        let (max_bsize, msize, fsize) = Self::calc_split(size);
        log::debug!("VMM usable size: {:#x}, metadata size: {:#x}", fsize, msize);
        let pages = msize.div_ceil(0x1000);
        for i in 0..pages {
            let frm = PMM.lock().frame();
            mpr.lock().map(frm, start + i * 0x1000, flags | PageTableFlags::WRITABLE | PageTableFlags::NO_EXECUTE);
        }
        let bitmap = unsafe { &mut *slice_from_raw_parts_mut(start as *mut u8, msize) };
        let mut this = Self {
            start: start + pages * 0x1000,
            max_bsize,
            flags,
            mpr,
            bitmap: BuddyBitmap::new(bitmap, max_bsize)
        };
        this.reserve_extra_areas(((0, 0), max_bsize), fsize);
        this
    }

    /// Calculates the maximum block size, metadata size, and free area size that will fit in the total size  
    /// Probably pretty slow but we only run it once so :/
    fn calc_split(mut size: usize) -> (usize, usize, usize) {
        fn get_max_bsize(size: usize) -> usize {
            let tmp = 2_usize.pow(size.ilog2());
            if tmp != size {
                tmp * 2
            } else {
                tmp
            }
        }

        size = (size / 0x1000) * 0x1000;
        let mut free_size = size - 0x1000; // Metadata must take up at least one page
        let mut max_bsize = get_max_bsize(free_size);
        let mut msize = BuddyBitmap::<0x1000>::size(max_bsize);
        while msize + free_size > size {
            free_size -= 0x1000;
            max_bsize = get_max_bsize(free_size);
            msize = BuddyBitmap::<0x1000>::size(max_bsize);
        }
        (max_bsize, msize, free_size)
    }

    fn reserve_extra_areas(&mut self, node: ((u32, usize), usize), org_size: usize) {
        let start = node.0.1 * node.1;
        let end = start + node.1;
        if start >= org_size {
            self.bitmap.set_status(node.0, BuddyStatus::USED);
        } else if end > org_size {
            self.reserve_extra_areas(((node.0.0 + 1, node.0.1 * 2), node.1 / 2), org_size);
            self.reserve_extra_areas(((node.0.0 + 1, node.0.1 * 2 + 1), node.1 / 2), org_size);
        }
    }

    pub fn allocate(&mut self, size: usize) -> NonNull<[u8]> {
        let bsize = size.next_power_of_two();
        if bsize > self.max_bsize { panic!("Allocation is larger than max block size"); }
        let mut best_block_split: Option<((u32, usize), usize)> = None;
        let block = self.alloc_traverse(((0, 0), self.max_bsize), bsize, &mut best_block_split);
        let block = block.or_else(|| best_block_split.map(|bbs| self.split_block(bbs, bsize)));
        if let Some(block) = block {
            self.bitmap.set_status(block.0, BuddyStatus::USED);
            let addr = self.start + block.1 * block.0.1;
            for i in 0..size.div_ceil(0x1000) {
                let frm = PMM.lock().frame();
                self.mpr.lock().map(frm, addr + i * 0x1000, self.flags);
            }
            return NonNull::slice_from_raw_parts(NonNull::new(addr as *mut u8).unwrap(), size);
        }
        panic!("Out of memory");
    }

    pub fn free(&mut self, ptr: NonNull<u8>, size: usize) {
        let bsize = size.next_power_of_two();
        let block = ((self.max_bsize / bsize).ilog2(), (ptr.as_ptr() as usize - self.start) / bsize);
        log::info!("Freeing block {:?}", block);
        self.bitmap.set_status(block, BuddyStatus::empty());
        self.merge_buddies(block);
    }

    fn alloc_traverse(&self, node: ((u32, usize), usize), tgt_bsize: usize, best_block_split: &mut Option<((u32, usize), usize)>) -> Option<((u32, usize), usize)> {
        let status = self.bitmap.get_status(node.0);
        if node.1 == tgt_bsize {
            if status.intersects(BuddyStatus::INTERNAL | BuddyStatus::USED) { return None }
            return Some(node);
        } else {
            if status.contains(BuddyStatus::USED) { return None }
            if status.contains(BuddyStatus::INTERNAL) {
                let mut block = self.alloc_traverse(((node.0.0 + 1, node.0.1 * 2), node.1 / 2), tgt_bsize, best_block_split);
                if block.is_some() { return block }
                block = self.alloc_traverse(((node.0.0 + 1, node.0.1 * 2 + 1), node.1 / 2), tgt_bsize, best_block_split);
                return block;
            }
            if let Some(bbs) = best_block_split {
                if node.1 < bbs.1 {
                    *bbs = node;
                }
            } else {
                *best_block_split = Some(node)
            }
            return None;
        }
    }

    fn split_block(&mut self, split_block: ((u32, usize), usize), tgt_bsize: usize) -> ((u32, usize), usize) {
        if split_block.1 == tgt_bsize {
            return split_block;
        }
        self.bitmap.set_status(split_block.0, BuddyStatus::INTERNAL);
        self.bitmap.set_status((split_block.0.0 + 1, split_block.0.1 + 1), BuddyStatus::empty()); // Leaf + free
        self.split_block(((split_block.0.0 + 1, split_block.0.1 * 2), split_block.1 / 2), tgt_bsize)
    }

    fn merge_buddies(&mut self, block: (u32, usize)) {
        // Return if root
        if block.0 == 0 { return; }
        let buddy = (block.0, block.1 ^ 1);
        let status = self.bitmap.get_status(buddy);
        if status.is_empty() {
            // Can merge
            let parent = (block.0 - 1, block.1 / 2);
            self.bitmap.set_status(parent, BuddyStatus::empty());
            self.merge_buddies(parent);
        }
    }
}

impl<const MIN_BSIZE: usize> BuddyBitmap<MIN_BSIZE> {
    pub fn new(bitmap: &'static mut [u8], max_bsize: usize) -> Self {
        bitmap.fill(0);
        let internal_depth = (max_bsize / MIN_BSIZE).ilog2();
        Self {
            bitmap,
            internal_blocks: 2_usize.pow(internal_depth)
        }
    }

    pub fn size(max_bsize: usize) -> usize {
        let internal_depth = (max_bsize / MIN_BSIZE).ilog2();
        let internal_blocks = 2_usize.pow(internal_depth);
        let bits = 2 * internal_blocks + max_bsize / MIN_BSIZE;
        bits.div_ceil(8)
    }

    pub(super) fn get_status(&self, block: (u32, usize)) -> BuddyStatus {
        let (int, off, mask) = self.translate(block);
        BuddyStatus::from_bits_truncate((self.bitmap[int] & (mask << off)) >> off)
    }
    
    pub(super) fn set_status(&mut self, block: (u32, usize), status: BuddyStatus) {
        let (int, off, mask) = self.translate(block);
        self.bitmap[int] &= !(mask << off); // Clear all the bits where we are setting
        self.bitmap[int] |= (status.bits() & mask) << off;
    }

    fn translate(&self, block: (u32, usize)) -> (usize, u8, u8) {
        let num_blocks = 2_usize.pow(block.0) + block.1;
        let (bit_off, mask) = if num_blocks < self.internal_blocks {
            (num_blocks * 2, 0b11)
        } else {
            (self.internal_blocks * 2 + (num_blocks - self.internal_blocks), 0b1)
        };
        return (
            bit_off / 8,
            (bit_off % 8) as u8,
            mask
        )
    }
}

pub static KVMM: Lazy<Mutex<VirtualMemoryManager>> = Lazy::new(|| {
    let start = get_mmap().iter().map(|m| m.length + m.base).max().unwrap();
    let size = get_mmap().iter()
        .filter(|m| m.entry_type == EntryType::USABLE)
        .map(|m| m.length).sum::<u64>();
    let vmm = VirtualMemoryManager::new(virt_addr(start as usize), size as usize, PageTableFlags::WRITABLE | PageTableFlags::NO_EXECUTE, &KMAPPER);
    log::info!("Initialized virtual memory manager");
    Mutex::new(vmm)
});