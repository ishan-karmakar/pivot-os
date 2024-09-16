use x86_64::{registers::control::Cr3, structures::paging::{Mapper, OffsetPageTable, Page, PageTable, PageTableFlags, PageTableIndex, PhysFrame, Size4KiB}, PhysAddr, VirtAddr};

use crate::{pmm::PMM, virt_addr, HH_OFF};

pub struct PTMapper<'a>(&'a mut PageTable);

impl<'a> PTMapper<'a> {
    pub fn new(pml4: usize) -> Self {
        Self(unsafe { &mut *(pml4 as *mut PageTable) })
    }

    pub fn map(&'a mut self, phys: usize, virt: usize, mut flags: PageTableFlags) {
        flags |= PageTableFlags::PRESENT;
        let virt = VirtAddr::new(virt as u64);
        let p3_tbl = Self::next_table(&mut self.0, virt.p4_index());
        let p2_tbl = Self::next_table(p3_tbl, virt.p3_index());
        if p2_tbl[virt.p2_index()].flags().contains(PageTableFlags::HUGE_PAGE) {}
    }

    fn next_table(table: &'a mut PageTable, idx: PageTableIndex) -> &'a mut PageTable {
        if table[idx].is_unused() {
            table[idx].set_addr(PhysAddr::new(PMM.lock().as_mut().unwrap().frame() as u64), PageTableFlags::PRESENT | PageTableFlags::WRITABLE);
        }
        unsafe { &mut *(virt_addr(table[idx].addr().as_u64() as usize) as *mut PageTable) }
    }
}

pub(crate) fn init() {
}