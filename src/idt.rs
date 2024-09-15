use core::ptr::{addr_of, read_unaligned};

use x86_64::{instructions::interrupts::int3, structures::idt::{ExceptionVector, InterruptDescriptorTable, InterruptStackFrameValue}, VirtAddr};

static mut IDT: InterruptDescriptorTable = InterruptDescriptorTable::new();

#[repr(C, packed)]
struct ISRStatus {
    pub restore_frame: InterruptStackFrameValue,
    pub int_no: u64
}

#[repr(C, packed)]
struct ISRStatusEC {
    pub restore_frame: InterruptStackFrameValue,
    pub ec: u64,
    pub int_no: u64
}

extern "C" {
    static isr_table: [VirtAddr; 256];
}

macro_rules! set_ec_handler {
    ($name:ident,$idx:literal) => {{
        IDT.$name.set_handler_addr(isr_table[$idx]).disable_interrupts(false);
        $idx
    }}
}

pub unsafe fn init() {
    let ec_ints = [
        set_ec_handler!(double_fault, 8),
        set_ec_handler!(invalid_tss, 10),
        set_ec_handler!(segment_not_present, 11),
        set_ec_handler!(stack_segment_fault, 12),
        set_ec_handler!(general_protection_fault, 13),
        set_ec_handler!(page_fault, 14),
        set_ec_handler!(alignment_check, 17),
        set_ec_handler!(machine_check, 18),
        set_ec_handler!(vmm_communication_exception, 29),
        set_ec_handler!(security_exception, 30)
    ];
    for i in 0..=255 {
        if ec_ints.contains(&i) || isr_table[i].is_null() { continue; }
        let options = IDT[i as u8].set_handler_addr(isr_table[i as usize]);
        if i < 32 {
            options.disable_interrupts(false); // Trap gate
        }
    }
    IDT.load();
    log::info!("Loaded interrupt descriptor table");
    int3();
}

#[no_mangle]
extern "C" fn exception_handler(status: ISRStatus) {
    panic!("Encountered exception {}", unsafe { read_unaligned(addr_of!(status.int_no)) });
}

#[no_mangle]
extern "C" fn exception_handler_ec(_status: ISRStatusEC) {
    panic!("Exception + EC");
}

#[no_mangle]
extern "C" fn irq_handler(_status: ISRStatus) {
    panic!("IRQ");
}
