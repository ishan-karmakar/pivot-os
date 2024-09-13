#![no_std]
#![no_main]

use core::panic::PanicInfo;

pub mod cpu;
pub mod limine;
pub mod serial;

#[no_mangle]
pub extern "C" fn kinit() -> ! {
    unsafe { cpu::set_int(false); }
    loop {}
}

#[panic_handler]
pub fn panic_handler(_info: &PanicInfo) -> ! {
    loop {}
}
