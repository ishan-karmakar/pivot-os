#![no_std]
#![no_main]

use core::panic::PanicInfo;

extern crate alloc;

#[macro_use]
pub mod writer;
pub mod cpu;
pub mod limine;
pub mod logger;
pub mod idt;
pub mod gdt;

/*
The entry point of the kernel.
Much of the components aren't actually shown here with the init functions.
This is because we use the spin::Lazy, which allows lazy initialization of statics.
Components such as the terminal, QEMU writer, SMP, etc. is all loaded on demand.
This simplifies the code a lot and avoids much of static muts.
*/
#[no_mangle]
pub extern "C" fn kinit() -> ! {
    cpu::init();
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
