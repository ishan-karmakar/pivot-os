use log::{LevelFilter, Log, SetLoggerError};

pub struct SimpleLogger;

impl Log for SimpleLogger {
    fn enabled(&self, _: &log::Metadata) -> bool {
        true
    }

    fn log(&self, record: &log::Record) {
        println!("[{}] {}: {}", record.level(), record.target(), record.args());
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