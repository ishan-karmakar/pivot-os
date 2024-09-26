#![no_std]
#![no_main]

use core::panic::PanicInfo;
extern crate alloc;

pub mod cpu;
pub mod limine;
pub mod logger;
pub mod writer;
pub mod idt;
pub mod gdt;

#[no_mangle]
pub extern "C" fn kinit() -> ! {
    cpu::init();
    pivot_drivers::qemu::init(); // Initialize the QEMU serial port + writer
    logger::init(log::LevelFilter::Debug).unwrap(); // Initialize logger + max level
    gdt::init_static();
    idt::init();
    pivot_mem::heap::init();
    gdt::init_dyn();
    loop {}
}

#[panic_handler]
pub fn panic_handler(info: &PanicInfo) -> ! {
    println!("{}", info);
    unsafe { pivot_util::cpu::set_int(false) };
    loop {}
}
