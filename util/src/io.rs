use core::arch::asm;

pub trait InOut {
    fn read(port: u16) -> Self;
    fn write(port: u16, value: Self);
}

impl InOut for u8 {
    fn read(port: u16) -> Self {
        let val: Self;
        unsafe { asm!("inb %al, %dx", in("dx") port, out("al") val) };
        val
    }

    fn write(port: u16, value: Self) {
        unsafe { asm!("outb %dx, %al", in("dx") port, in("al") value) };
    }
}

impl InOut for u16 {
    fn read(port: u16) -> Self {
        let val: Self;
        unsafe { asm!("inw %ax, %dx", in("dx") port, out("ax") val) };
        val
    }

    fn write(port: u16, value: Self) {
        unsafe { asm!("outw %dx, %ax", in("dx") port, in("ax") value) };
    }
}

impl InOut for u32 {
    fn read(port: u16) -> Self {
        let val: Self;
        unsafe { asm!("inb %eax, %dx", in("dx") port, out("eax") val) };
        val
    }

    fn write(port: u16, value: Self) {
        unsafe { asm!("outb %dx, %eax", in("dx") port, in("eax") value) };
    }
}