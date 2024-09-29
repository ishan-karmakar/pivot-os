use core::{arch::asm, ptr::NonNull};

use acpi::{AcpiHandler, AcpiTables, PhysicalMapping};
use aml::{AmlContext, Handler};
use limine::request::RsdpRequest;
use pivot_mem::virt_addr;
use pivot_util::io::InOut;
use spin::{Lazy, Mutex};

#[derive(Clone)]
struct KernelAcpiHandler;
impl AcpiHandler for KernelAcpiHandler {
    unsafe fn map_physical_region<T>(&self, physical_address: usize, size: usize) -> PhysicalMapping<Self, T> {
        PhysicalMapping::new(
            physical_address,
            unsafe { NonNull::new_unchecked(virt_addr(physical_address) as *mut T) },
            size,
            size.next_multiple_of(0x1000),
            KernelAcpiHandler {}
        )
    }

    fn unmap_physical_region<T>(_: &PhysicalMapping<Self, T>) {
    }
}

struct KernelAMLHandler;

impl KernelAMLHandler {
    fn read<T: Copy>(&self, address: usize) -> T {
        unsafe { *(address as *const T) }
    }

    fn write<T>(&self, address: usize, val: T) {
        unsafe { *(address as *mut T) = val }
    }

    fn read_pci<T: Copy>(&self, segment: u16, bus: u8, device: u8, function: u8, offset: u16) -> T {
        InOut::write(0xCF8, ((bus << 16) as u32) | (device << 11) | (function << 8) | (offset & 0xFC) | 0x80000000);
    }
}

impl Handler for KernelAMLHandler {
    fn read_u8(&self, address: usize) -> u8 { self.read::<u8>(address) }
    fn read_u16(&self, address: usize) -> u16 { self.read::<u16>(address) }
    fn read_u32(&self, address: usize) -> u32 { self.read::<u32>(address) }
    fn read_u64(&self, address: usize) -> u64 { self.read::<u64>(address) }

    fn write_u8(&mut self, address: usize, value: u8) { self.write(address, value) }
    fn write_u16(&mut self, address: usize, value: u16) { self.write(address, value) }
    fn write_u32(&mut self, address: usize, value: u32) { self.write(address, value) }
    fn write_u64(&mut self, address: usize, value: u64) { self.write(address, value) }

    fn read_io_u8(&self, port: u16) -> u8 { InOut::read(port) }
    fn read_io_u16(&self, port: u16) -> u16 { InOut::read(port) }
    fn read_io_u32(&self, port: u16) -> u32 { InOut::read(port) }

    fn write_io_u8(&self, port: u16, value: u8) { InOut::write(port, value) }
    fn write_io_u16(&self, port: u16, value: u16) { InOut::write(port, value) }
    fn write_io_u32(&self, port: u16, value: u32) { InOut::write(port, value) }

    fn read_pci_u8(&self, segment: u16, bus: u8, device: u8, function: u8, offset: u16) -> u8 {
    }
}

#[link_section = ".requests"]
static RSDP_REQUEST: RsdpRequest = RsdpRequest::new();

static TABLES: Mutex<Lazy<AcpiTables<KernelAcpiHandler>>> = Mutex::new(Lazy::new(|| unsafe { AcpiTables::from_rsdp(KernelAcpiHandler {}, RSDP_REQUEST.get_response().unwrap().address() as usize).unwrap() }));

pub fn init() {
}