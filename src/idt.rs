use x86_64::{registers::control::Cr2, structures::idt::{InterruptDescriptorTable, InterruptStackFrame, InterruptStackFrameValue}, VirtAddr};

use crate::cpu;

static mut IDT: InterruptDescriptorTable = InterruptDescriptorTable::new();

#[repr(C)]
struct ExceptionStatus {
    pub status: cpu::Status,
    pub int_no: u64,
    pub restore_frame: InterruptStackFrameValue
}

#[repr(C)]
struct ExceptionStatusEC {
    pub status: cpu::Status,
    pub int_no: u64,
    pub ec: u64,
    pub restore_frame: InterruptStackFrameValue
}

#[repr(C)]
struct IRQStatus {
    pub cr3: u64,
    pub status: cpu::Status,
    pub int_no: u64,
    pub restore_frame: InterruptStackFrame
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

pub fn init() {
    unsafe {
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
        for i in 0..32 {
            if ec_ints.contains(&i) || isr_table[i].is_null() { continue; }
            IDT[i as u8].set_handler_addr(isr_table[i]).disable_interrupts(false);
        }
        for i in 32..=255 {
            IDT[i].set_handler_addr(isr_table[i as usize]);
        }
        IDT.load();
    }
    log::info!("Loaded interrupt descriptor table");
}

fn log_registers(st: &cpu::Status, rf: &InterruptStackFrameValue) {
    log::debug!("SS: {:#x}, RSP: {:#x}, RFLAGS: {:#x}, CS: {:#x}",
        rf.stack_segment.0, rf.stack_pointer, rf.cpu_flags, rf.code_segment.0);
    log::debug!("RIP: {:#x}, RAX: {:#x}, RBX: {:#x}, RCX: {:#x}",
        rf.instruction_pointer, st.rax, st.rbx, st.rcx);
    log::debug!("RDX: {:#x}, RBP: {:#x}, RSI: {:#x}, RDI: {:#x}",
        st.rdx, st.rbp, st.rsi, st.rdi);
    log::debug!("R8: {:#x}, R9: {:#x}, R10: {:#x}, R11: {:#x}",
        st.r8, st.r9, st.r10, st.r11);
    log::debug!("R12: {:#x}, R13: {:#x}, R14: {:#x}, R15: {:#x}",
        st.r12, st.r13, st.r14, st.r15);
}

#[no_mangle]
extern "C" fn exception_handler(status: &ExceptionStatus) -> ! {
    log::error!("Received exception {}", status.int_no);
    log_registers(&status.status, &status.restore_frame);
    panic!();
}

#[no_mangle]
extern "C" fn exception_handler_ec(status: &ExceptionStatusEC) -> ! {
    log::error!("Received exception {} with error code {}", status.int_no, status.ec);
    if status.int_no == 14 {
        log::debug!("CR2: {:?}", Cr2::read().unwrap());
    }
    log_registers(&status.status, &status.restore_frame);
    panic!();
}

#[no_mangle]
extern "C" fn irq_handler(_status: &IRQStatus) -> &IRQStatus {
    panic!("IRQ");
}
