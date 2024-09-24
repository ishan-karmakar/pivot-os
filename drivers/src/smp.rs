extern crate alloc;
use limine::{request::SmpRequest, smp::RequestFlags};
use alloc::vec::Vec;

pub struct Cpu {
}

#[link_section = ".requests"]
pub static SMP_REQUEST: SmpRequest = SmpRequest::new().with_flags(RequestFlags::X2APIC);

pub static CPUS: Vec<Cpu> = Vec::new();

pub fn early_init() {

}