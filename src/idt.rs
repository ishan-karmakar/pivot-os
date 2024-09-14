use core::ffi::c_void;

use x86_64::structures::idt::InterruptDescriptorTable;

static mut IDT: InterruptDescriptorTable = InterruptDescriptorTable::new();

// extern "C" {
//     pub static isr_table: [usize; 256];
// }

#[no_mangle]
#[used]
pub static TEST: u32 = 5;

pub unsafe fn init() {
}
