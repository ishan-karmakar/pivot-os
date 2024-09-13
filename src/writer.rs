use core::fmt::{Arguments, Write};

use crate::qemu;

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
    unsafe { qemu::QEMU_WRITER.write_fmt(args).unwrap(); }
}