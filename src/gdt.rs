use core::cmp::max;

use x86_64::{instructions::tables::lgdt, registers::segmentation::{Segment, CS, DS, ES, FS, GS, SS}, structures::{gdt::{Descriptor, SegmentSelector}, DescriptorTablePointer}, PrivilegeLevel, VirtAddr};

struct GlobalDescriptorTable {
    buffer: *mut u64,
    len: usize
}

unsafe impl Sync for GlobalDescriptorTable {}

impl GlobalDescriptorTable {
    // Caller is responsible for making sure buffer[0] is a null descriptor
    pub const fn new(buffer: *mut u64, len: usize) -> Self {
        Self { buffer, len }
    }

    pub unsafe fn set(&mut self, idx: u16, entry: Descriptor) {
        let idx = idx as usize;
        match entry {
            Descriptor::UserSegment(val) => {
                *self.buffer.add(idx) = val;
                self.len = max(idx + 1, self.len);
            }
            Descriptor::SystemSegment(low, high) => {
                *self.buffer.add(idx) = low;
                *self.buffer.add(idx + 1) = high;
                self.len = max(idx + 2, self.len);
            }
        };
    }

    pub unsafe fn load(&'static self) {
        let gdtr = DescriptorTablePointer {
            base: VirtAddr::new(self.buffer as u64),
            limit: (self.len * size_of::<u64>() - 1) as u16
        };
        lgdt(&gdtr);
    }
}

// This is the initial GDT used by our kernel before the heap is initialized
// It contains the bare minimum needed to handle interrupts
static mut STATIC_GDT: [u64; 3] = [0; 3];

static mut GDT: GlobalDescriptorTable = unsafe { GlobalDescriptorTable::new(STATIC_GDT.as_mut_ptr(), 3) };

pub static KCODE: SegmentSelector = SegmentSelector::new(1, PrivilegeLevel::Ring0);
pub static KDATA: SegmentSelector = SegmentSelector::new(2, PrivilegeLevel::Ring0);

pub fn init_static() {
    unsafe {
        GDT.set(1, Descriptor::kernel_code_segment());
        GDT.set(2, Descriptor::kernel_data_segment());
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

pub fn init_dyn() {}