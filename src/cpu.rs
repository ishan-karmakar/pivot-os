use pivot_util::cpu;

pub fn init() {
    unsafe { cpu::set_int(false) };
    cpu::wrgsbase(0);
}