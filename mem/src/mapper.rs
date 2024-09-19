use core::ops::{Deref, DerefMut};

use spin::{Mutex, MutexGuard};
use x86_64::{registers::control::Cr3, structures::paging::{page_table::PageTableEntry, PageTable, PageTableFlags}, PhysAddr, VirtAddr};

use crate::{pmm::PMM, virt_addr};

pub struct PTMapper<'a>(&'a mut PageTable);

pub struct LockedPTMapper<'a>(Mutex<Option<PTMapper<'a>>>);

impl<'a> PTMapper<'a> {
    const HP_SIZE: usize = 0x20000;
    const TBL_FLAGS: PageTableFlags = PageTableFlags::PRESENT.union(PageTableFlags::WRITABLE);

    pub fn new(pml4: usize) -> Self {
        Self(unsafe { &mut *(virt_addr(pml4) as *mut PageTable) })
    }

    pub fn map(&mut self, phys: usize, virt: usize, mut flags: PageTableFlags) {
        flags |= PageTableFlags::PRESENT;
        let virta = VirtAddr::new(virt as u64);
        let physa = PhysAddr::new(phys as u64);
        let p3_tbl = Self::next_table(&mut self.0[virta.p4_index()]);
        let p2_tbl = Self::next_table(&mut p3_tbl[virta.p3_index()]);
        let p2_ent = &mut p2_tbl[virta.p2_index()];
        /*
        If mapping is already HP:
            If existing aligned phys == target aligned phys and phys % HP size == virt % HP size:
                return
            If mapping doesn't work for us:
                convert to non HP mapping (create P1 tbl + copy all existing mappings over)
        If mapping doesn't exist:
            If phys % HP size == virt % HP size:
                Use HP mapping
            If mapping doesn't work for us:
                create P1 tbl and use it instead
         */
        let hp_works = phys % Self::HP_SIZE == virt % Self::HP_SIZE;
        let p1_tbl = if p2_ent.flags().contains(PageTableFlags::HUGE_PAGE) {
            if physa.align_down(Self::HP_SIZE as u64) == p2_ent.addr() && hp_works {
                None
            } else {
                // Convert to non HP mapping
                let tbl_phys = PMM.lock().frame();
                let tbl = unsafe { &mut *(virt_addr(tbl_phys) as *mut PageTable) };
                for i in 0..512 {
                    tbl[i].set_addr(p2_ent.addr() + i as u64 * 0x1000u64, p2_ent.flags() & !PageTableFlags::HUGE_PAGE);
                }
                p2_ent.set_addr(PhysAddr::new(tbl_phys as u64), Self::TBL_FLAGS);
                Some(tbl)
            }
        } else if p2_ent.is_unused() && hp_works {
            p2_ent.set_addr(physa, Self::TBL_FLAGS | PageTableFlags::HUGE_PAGE);
            None
        } else {
            Some(Self::next_table(p2_ent))
        };
        if let Some(p1_tbl) = p1_tbl {
            p1_tbl[virta.p1_index()].set_addr(physa, flags);
        }
    }

    fn next_table(ent: &mut PageTableEntry) -> &'a mut PageTable {
        // OPTIMIZATION - If we ever need to create a table we now know that all the future tables will need to be created
        // There is no point in the ent.is_unused() check once we create a table
        if ent.is_unused() {
            let frm = PMM.lock().frame();
            ent.set_addr(PhysAddr::new(frm as u64), Self::TBL_FLAGS);
            let tbl = unsafe { &mut *(virt_addr(frm) as *mut PageTable) };
            for ent in tbl.iter_mut() {
                ent.set_unused();
            }
            tbl
        } else {
            unsafe { &mut *(virt_addr(ent.addr().as_u64() as usize) as *mut PageTable) }
        }
    }
}

pub static KMAPPER: Mutex<Option<PTMapper>> = Mutex::new(None);

pub(crate) fn init() {
    let cr3 = Cr3::read().0.start_address().as_u64() as usize;
    log::debug!("CR3: {:#x}", cr3);
    *KMAPPER.lock() = Some(PTMapper::new(cr3));
    log::info!("Initialized kernel mapper");
}