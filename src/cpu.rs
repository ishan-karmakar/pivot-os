use core::arch::asm;

#[repr(C)]
pub struct Status {
    pub r15: u64,
    pub r14: u64,
    pub r13: u64,
    pub r12: u64,
    pub r11: u64,
    pub r10: u64,
    pub r9: u64,
    pub r8: u64,
    pub rdi: u64,
    pub rsi: u64,
    pub rbp: u64,
    pub rdx: u64,
    pub rcx: u64,
    pub rbx: u64,
    pub rax: u64
}

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