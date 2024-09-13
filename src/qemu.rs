use core::fmt::Write;

use uart_16550::SerialPort;

pub struct QEMUWriter(pub SerialPort);

pub static mut QEMU_WRITER: QEMUWriter = QEMUWriter(unsafe { SerialPort::new(0x3F8) });

impl Write for QEMUWriter {
    fn write_char(&mut self, c: char) -> core::fmt::Result {
        self.0.send(c as u8);
        Ok(())
    }

    fn write_str(&mut self, s: &str) -> core::fmt::Result {
        for c in s.chars() {
            self.write_char(c)?;
        }
        Ok(())
    }
}
