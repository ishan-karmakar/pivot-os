#![no_std]
#![no_main]
#![feature(allocator_api)]

use core::panic::PanicInfo;

use pivot_mem::vmm::KVMM;

pub mod cpu;
pub mod limine;
pub mod logger;
pub mod writer;
pub mod idt;
pub mod gdt;

#[no_mangle]
pub extern "C" fn kinit() -> ! {
    unsafe { cpu::set_int(false) }; // Disable all interrupts until we are ready to handle them
    pivot_drivers::qemu::init(); // Initialize the QEMU serial port + writer
    logger::init(log::LevelFilter::Debug).unwrap(); // Initialize logger + max level
    unsafe { gdt::init_static() };
    unsafe { idt::init() };
    KVMM.lock();
    // pivot_mem::heap::init();
    // let test = Box::new(5);
    loop {}
}

#[panic_handler]
pub fn panic_handler(info: &PanicInfo) -> ! {
    println!("{}", info);
    unsafe { cpu::set_int(false) };
    loop {}
}
