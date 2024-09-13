use core::arch::asm;

pub unsafe fn set_int(int: bool) {
    if int {
        asm!("sti");
    } else {
        asm!("cli");
    }
}

pub unsafe fn inb(port: u16, val: u8) {
    asm!("inb al, dx", in("al") val, in("dx") port);
}

pub unsafe fn outb(port: u16, val: u8) {
    asm!("outb dx, al", in("dx") port, in("al") val);
}