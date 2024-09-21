use x86_64::{registers::control::Cr3, structures::paging::{page_table::PageTableEntry, PageTable, PageTableFlags}, PhysAddr, VirtAddr};

use crate::{pmm::PhysicalMemoryManager, virt_addr};

pub struct PTMapper(*mut PageTable);
// pub struct LockedPTMapper(Mutex<Option<PTMapper>>);

impl PTMapper {
    const HP_SIZE: usize = 0x20000;
    const TBL_FLAGS: PageTableFlags = PageTableFlags::PRESENT.union(PageTableFlags::WRITABLE);

    pub fn new(pml4: usize) -> Self {
        Self(unsafe { &mut *(virt_addr(pml4) as *mut PageTable) })
    }

    pub fn map(&mut self, phys: usize, virt: usize, mut flags: PageTableFlags, pmm: &mut PhysicalMemoryManager) {
        flags |= PageTableFlags::PRESENT;
        let virta = VirtAddr::new(virt as u64);
        let physa = PhysAddr::new(phys as u64);
        let p3_tbl = Self::next_table(unsafe { &mut (*self.0)[virta.p4_index()] }, pmm);
        let p2_tbl = Self::next_table(&mut p3_tbl[virta.p3_index()], pmm);
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
                let tbl_phys = pmm.frame();
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
            Some(Self::next_table(p2_ent, pmm))
        };
        if let Some(p1_tbl) = p1_tbl {
            p1_tbl[virta.p1_index()].set_addr(physa, flags);
        }
    }

    pub fn unmap(&mut self, virt: usize) {
        // TODO: Free unused pages
        let virt = VirtAddr::new(virt as u64);
        // Making a dummy PMM because we make sure that the tables exist - We should never have to actually use the PMM
        let pmm = unsafe { &mut *core::ptr::null_mut::<PhysicalMemoryManager>() };
        // Checks if unused twice for each new page table
        // But unused is just == 0 so I don't really care
        let p4_ent = unsafe { &mut (*self.0)[virt.p4_index()] };
        if p4_ent.is_unused() { return; }
        let p3_tbl = Self::next_table(p4_ent, pmm);
        let p3_ent = &mut p3_tbl[virt.p3_index()];
        if p3_ent.is_unused() { return; }
        let p2_tbl = Self::next_table(p3_ent, pmm);
        let p2_ent = &mut p2_tbl[virt.p2_index()];
        if p2_ent.is_unused() || p2_ent.flags().contains(PageTableFlags::HUGE_PAGE) { return; }
        let p1_tbl = Self::next_table(p2_ent, pmm);
        p1_tbl[virt.p1_index()].set_unused();
    }

    // Gets the physical address given a virtual address. Returns 0 if intermediate page table doesn't exist
    pub fn translate(&mut self, virt: usize) -> usize {
        let virt = VirtAddr::new(virt as u64);
        let pmm = unsafe { &mut *core::ptr::null_mut::<PhysicalMemoryManager>() };
        // Checks if unused twice for each new page table
        // But unused is just == 0 so I don't really care
        let p4_ent = unsafe { &mut (*self.0)[virt.p4_index()] };
        if p4_ent.is_unused() { return 0; }
        let p3_tbl = Self::next_table(p4_ent, pmm);
        let p3_ent = &mut p3_tbl[virt.p3_index()];
        if p3_ent.is_unused() { return 0; }
        let p2_tbl = Self::next_table(p3_ent, pmm);
        let p2_ent = &mut p2_tbl[virt.p2_index()];
        if p2_ent.is_unused() || p2_ent.flags().contains(PageTableFlags::HUGE_PAGE) {
            return (p2_ent.addr() + virt.as_u64() % 0x20000).as_u64() as usize;
        }
        let p1_tbl = Self::next_table(p2_ent, pmm);
        return p1_tbl[virt.p1_index()].addr().as_u64() as usize;
    }

    fn next_table(ent: &mut PageTableEntry, pmm: &mut PhysicalMemoryManager) -> &'static mut PageTable {
        // OPTIMIZATION - If we ever need to create a table we now know that all the future tables will need to be created
        // There is no point in the ent.is_unused() check once we create a table
        if ent.is_unused() {
            let frm = pmm.frame();
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

// impl LockedPTMapper {
//     pub const fn new() -> Self { Self(Mutex::new(None)) }
//     pub fn init(&self, pml4: usize) { *self.0.lock() = Some(PTMapper::new(pml4)); }
//     pub fn map(&self, phys: usize, virt: usize, flags: PageTableFlags) { self.0.lock().as_mut().unwrap().map(phys, virt, flags) }
//     pub fn unmap(&self, virt: usize) { self.0.lock().as_mut().unwrap().unmap(virt) }
//     pub fn translate(&self, virt: usize) -> usize { self.0.lock().as_mut().unwrap().translate(virt) }
// }

// pub static KMAPPER: LockedPTMapper = LockedPTMapper::new();

pub(crate) fn init() -> PTMapper {
    let cr3 = Cr3::read().0.start_address().as_u64() as usize;
    log::debug!("CR3: {:#x}", cr3);
    // KMAPPER.init(cr3);
    log::info!("Initialized kernel mapper");
    PTMapper::new(cr3)
}