use core::fmt::{Arguments, Write};

use pivot_drivers::{qemu::QEMU_WRITER, term::TERM_WRITER};

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
    QEMU_WRITER.lock().write_fmt(args).unwrap();
    TERM_WRITER.lock().write_fmt(args).unwrap();
}