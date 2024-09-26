use alloc::{boxed::Box, vec::Vec};
use limine::{request::SmpRequest, smp::RequestFlags};
use pivot_util::cpu;
use spin::Lazy;
use x86_64::registers::segmentation::{Segment64, GS};

pub struct CpuData {
    pub lapic_id: u32
}

impl CpuData {
    pub const fn new(lapic_id: u32) -> Self {
        Self {
            lapic_id
        }
    }
}

#[link_section = ".requests"]
pub static mut SMP_REQUEST: SmpRequest = SmpRequest::new().with_flags(RequestFlags::X2APIC);

pub static CPUS: Lazy<Box<[CpuData]>> = Lazy::new(|| {
    let response = unsafe { SMP_REQUEST.get_response_mut() }.unwrap();
    let bsp = response.bsp_lapic_id();
    let limine_cpus = response.cpus_mut();
    let mut cpus: Vec<CpuData> = Vec::with_capacity(limine_cpus.len());
    log::info!("Number of CPUs: {}", limine_cpus.len());
    for (i, cpu) in limine_cpus.iter_mut().enumerate() {
        cpus.push(CpuData::new(cpu.lapic_id));
        cpu.extra = &cpus[i] as *const _ as u64;
        if cpu.lapic_id == bsp {
            cpu::wrgsbase(cpu.extra);
        }
    }
    cpus.into_boxed_slice()
});

pub fn this_cpu() -> &'static mut CpuData {
    return unsafe { &mut *GS::read_base().as_mut_ptr::<CpuData>() };
}
