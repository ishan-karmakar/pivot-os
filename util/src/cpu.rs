use core::arch::{asm, x86_64::__cpuid_count};

use spin::Lazy;
use x86_64::{registers::{control::{Cr4, Cr4Flags}, model_specific::GsBase, segmentation::{Segment64, GS}}, VirtAddr};

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

static GS_ACCESS_METHOD: Lazy<bool> = Lazy::new(|| {
    let wrbase_supported = unsafe { __cpuid_count(7, 0).ebx } & 1 > 0;
    if wrbase_supported {
        unsafe { Cr4::update(|f| *f |= Cr4Flags::FSGSBASE) };
    }
    wrbase_supported
});

pub unsafe fn set_int(int: bool) {
    if int {
        asm!("sti");
    } else {
        asm!("cli");
    }
}

pub fn wrgsbase(val: u64) {
    let val = VirtAddr::new(val);
    if *GS_ACCESS_METHOD {
        unsafe { GS::write_base(val) };
    } else {
        GsBase::write(val);
    }
}

pub fn rdgsbase() -> u64 {
    if *GS_ACCESS_METHOD {
        GS::read_base().as_u64()
    } else {
        GsBase::read().as_u64()
    }
}
