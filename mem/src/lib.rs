#![no_std]

use limine::request::HhdmRequest;
use spin::Lazy;

pub mod pmm;
pub mod mapper;

#[used]
#[link_section = ".requests"]
static HHDM_REQUEST: HhdmRequest = HhdmRequest::new();

static HH_OFF: Lazy<usize> = Lazy::new(|| HHDM_REQUEST.get_response().unwrap().offset() as usize);

pub fn virt_addr(phys: usize) -> usize {
    if phys >= *HH_OFF { return phys; }
    phys + *HH_OFF
}

pub fn phys_addr(virt: usize) -> usize {
    if virt < *HH_OFF { return virt; }
    virt - *HH_OFF
}

pub unsafe fn init() {
    pmm::init();
    mapper::init();
}