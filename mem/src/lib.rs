#![no_std]

use limine::request::HhdmRequest;
use spin::Lazy;

pub mod pmm;
pub mod mapper;
pub mod vmm;

macro_rules! unwrap_mutex {
    ($expression:expr) => { $expression.lock().as_mut().unwrap() }
}
pub(crate) use unwrap_mutex;

#[used]
#[link_section = ".requests"]
static HHDM_REQUEST: HhdmRequest = HhdmRequest::new();

static HH_OFF: Lazy<usize> = Lazy::new(|| HHDM_REQUEST.get_response().unwrap().offset() as usize);

#[inline]
pub fn get_hh_off() -> usize { *HH_OFF }

pub fn virt_addr(phys: usize) -> usize {
    if phys >= get_hh_off() { return phys; }
    phys + get_hh_off()
}

pub fn phys_addr(virt: usize) -> usize {
    if virt < get_hh_off() { return virt; }
    virt - get_hh_off()
}

pub unsafe fn init() {
    pmm::init();
    mapper::init();
    vmm::init();
}