#![no_std]
#![no_main]
#![feature(abi_x86_interrupt)]
#![feature(naked_functions)]

use core::panic::PanicInfo;

pub mod cpu;
pub mod limine;
pub mod qemu;
pub mod logger;
pub mod writer;
pub mod idt;

#[no_mangle]
pub extern "C" fn kinit() -> ! {
    unsafe { cpu::set_int(false); }
    unsafe { qemu::QEMU_WRITER.0.init(); }
    logger::init(log::LevelFilter::Debug).unwrap();
    loop {}
}

#[panic_handler]
pub fn panic_handler(_info: &PanicInfo) -> ! {
    loop {}
}
