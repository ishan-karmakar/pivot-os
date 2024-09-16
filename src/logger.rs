use log::{LevelFilter, Log, SetLoggerError};
use core::fmt::Write;

pub struct SimpleLogger;

impl Log for SimpleLogger {
    fn enabled(&self, _: &log::Metadata) -> bool {
        true
    }

    fn log(&self, record: &log::Record) {
        // If terminal is not initialized
        unsafe { writeln!(pivot_drivers::qemu::QEMU_WRITER, "[{}] {}: {}", record.level(), record.target(), record.args()) }.unwrap();
        // else write to terminal
    }

    fn flush(&self) {}
}

pub static LOGGER: SimpleLogger = SimpleLogger;

pub fn init(l: LevelFilter) -> Result<(), SetLoggerError> {
    log::set_logger(&LOGGER)?;
    log::set_max_level(l);
    log::info!("Initialized logger");
    Ok(())
}