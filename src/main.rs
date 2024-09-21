#![no_std]
#![no_main]

use core::panic::PanicInfo;

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
    let pmm = pivot_mem::pmm::init();
    let mpr = pivot_mem::mapper::init(&pmm);
    let vmm = pivot_mem::vmm::init(&mpr, &pmm);
    let heap = pivot_mem::heap::init();
    loop {}
}

#[panic_handler]
pub fn panic_handler(info: &PanicInfo) -> ! {
    println!("{}", info);
    unsafe { cpu::set_int(false) };
    loop {}
}
