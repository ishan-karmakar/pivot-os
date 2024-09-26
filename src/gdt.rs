use core::cmp::max;

use alloc::vec;
use pivot_drivers::smp::CPUS;
use x86_64::{instructions::tables::lgdt, registers::segmentation::{Segment, CS, DS, ES, FS, GS, SS}, structures::{gdt::{Descriptor, SegmentSelector}, DescriptorTablePointer}, PrivilegeLevel, VirtAddr};

struct GlobalDescriptorTable {
    buffer: *mut u64,
    len: u16
}

unsafe impl Sync for GlobalDescriptorTable {}

impl GlobalDescriptorTable {
    // Caller is responsible for making sure buffer[0] is a null descriptor
    pub const fn new(buffer: *mut u64) -> Self {
        Self { buffer, len: 1 }
    }

    // TODO: Remove if only using push
    unsafe fn set(&mut self, idx: u16, entry: Descriptor) {
        match entry {
            Descriptor::UserSegment(val) => {
                *self.buffer.add(idx as usize) = val;
                self.len = max(idx + 1, self.len);
            }
            Descriptor::SystemSegment(low, high) => {
                *self.buffer.add(idx as usize) = low;
                *self.buffer.add(idx as usize + 1) = high;
                self.len = max(idx + 2, self.len);
            }
        };
    }

    unsafe fn push(&mut self, entry: Descriptor) {
        self.set(self.len, entry);
    }

    unsafe fn load(&'static self) {
        let gdtr = DescriptorTablePointer {
            base: VirtAddr::new(self.buffer as u64),
            limit: self.len * size_of::<u64>() as u16 - 1
        };
        lgdt(&gdtr);
    }
}

// This is the initial GDT used by our kernel before the heap is initialized
// It contains the bare minimum needed to handle interrupts
static mut STATIC_GDT: [u64; 3] = [0; 3];

static mut GDT: GlobalDescriptorTable = unsafe { GlobalDescriptorTable::new(STATIC_GDT.as_mut_ptr()) };

pub static KCODE: SegmentSelector = SegmentSelector::new(1, PrivilegeLevel::Ring0);
pub static KDATA: SegmentSelector = SegmentSelector::new(2, PrivilegeLevel::Ring0);

pub fn init_static() {
    unsafe {
        GDT.push(Descriptor::kernel_code_segment());
        GDT.push(Descriptor::kernel_data_segment());
        GDT.load();
        CS::set_reg(KCODE);
        DS::set_reg(KDATA);
        SS::set_reg(KDATA);
        ES::set_reg(KDATA);
        FS::set_reg(KDATA);
        GS::set_reg(KDATA);
    }
    log::info!("Initialized static GDT");
}

// Initialized dynamic GDT
pub fn init_dyn() {
    let mut boxed_gdt = vec![0u64; 5 + CPUS.len()].into_boxed_slice();
    unsafe {
        GDT = GlobalDescriptorTable::new(boxed_gdt.as_mut_ptr());
        GDT.push(Descriptor::kernel_code_segment());
        GDT.push(Descriptor::kernel_data_segment());
        GDT.push(Descriptor::user_code_segment());
        GDT.push(Descriptor::user_data_segment());
        GDT.load();
    };
}