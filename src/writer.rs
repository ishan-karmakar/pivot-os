use core::fmt::{Arguments, Write};

#[macro_export]
macro_rules! print {
    ($($arg:tt)*) => ($crate::writer::_print(format_args!($($arg)*)));
}

#[macro_export]
macro_rules! println {
    () => (print!("\n"));
    ($($arg:tt)*) => (print!("{}\n", format_args!($($arg)*)));
}

pub fn _print(args: Arguments) {
    unsafe { pivot_drivers::qemu::QEMU_WRITER.write_fmt(args).unwrap(); }
    // TODO: Add framebuffer when implemented
}