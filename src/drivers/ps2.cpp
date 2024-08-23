// #include <io/serial.hpp>
// #include <acpi/fadt.hpp>
// #include <drivers/ps2.hpp>
// #include <lib/logger.hpp>
// using namespace drivers;

// bool PS2::dchannel;

// void PS2::init() {
//     // Check if PS2 controller exists in FADT
//     // TODO: Figure out why this isn't working
//     // Maybe QEMU isn't emulating correctly, but boot_arch_flags is always clear
//     // auto fadt = acpi::ACPI::get_table<acpi::FADT>().value();
//     // if (!(fadt.table->boot_arch_flags & 0b10))
//     //     return log(INFO, "PS2", "No PS/2 controller exists, skipping initialization");

//     // Disable both channels
//     io::outb(PORT, 0xAD);
//     io::outb(PORT, 0xA7);

//     // Flush buffer (inb ...)
//     while (io::inb(PORT) & 1) asm ("pause");

//     // Self test
//     io::outb(PORT, 0xAA);
//     if (io::inb(PORT) != 0x55)
//         return log(ERROR, "PS/2", "PS/2 controller failed self check");

//     // Set config byte
//     uint8_t cfg = get_config();
//     set_config(cfg & 0b00110100);
//     dchannel = cfg & (1 << 5);

//     // Check second channel
//     if (dchannel) {
//         io::outb(PORT, 0xA8);
//         dchannel = !(get_config() & (1 << 5));
//         if (dchannel)
//             io::outb(PORT, 0xA7);
//     }

//     // Interface tests
//     io::outb(PORT, 0xAB);
//     if (io::inb(PORT))
//         return log(ERROR, "PS/2", "Port 1 failed self test");
    
//     if (dchannel) {
//         io::outb(PORT, 0xA9);
//         if (io::inb(PORT)) {
//             logger::warning("PS/2", "Port 2 failed self test");
//             dchannel = false;
//         }
//     }

//     // Enable devices and interrupts
//     cfg = get_config() | 1;
//     io::outb(PORT, 0xAE);
//     if (dchannel) {
//         io::outb(PORT, 0xA8);
//         cfg |= 0b10;
//     }
//     set_config(cfg);

//     // Reset devices
//     io::outb(DATA, 0xFF);
// }

// uint8_t PS2::get_config() {
//     io::outb(PORT, 0x20);
//     return io::inb(PORT);
// }

// void PS2::set_config(uint8_t byte) {
//     io::outb(PORT, 0x60);
//     io::outb(PORT, byte);
// }

// void PS2::send_p1(uint8_t byte) {
//     //
// }