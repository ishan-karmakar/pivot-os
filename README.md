# PivotOS

A hobby x86-64 operating system kernel written in Zig, built with the [Limine bootloader](https://limine-bootloader.org/).

## Overview

PivotOS is an experimental kernel project focusing on x86-64 architecture with support for modern hardware features including ACPI, multi-processor (SMP) support, and network capabilities via lwIP.

## Features

- **x86-64 Architecture**: Full support for 64-bit x86 processors
- **Limine Bootloader**: UEFI and BIOS boot support
- **ACPI Support**: Hardware power management via uACPI
- **SMP Enabled**: Multi-processor execution support
- **Network Stack**: lwIP (Lightweight IP) integration for TCP/IP networking
- **Hardware Drivers**: Support for various components including:
  - Framebuffer (video output)
  - IDE disk drives
  - PCI and PCIe devices
  - LAPIC (Local APIC) and I/O APIC
  - HPET, PIT, TSC timers
  - MSI/MSI-X interrupt handling
- **Memory Management**: Virtual and physical memory management
- **Modular Design**: Organized driver and library architecture

## Requirements

- **Zig**: Latest version (0.13.0+)
- **Build Tools**: 
  - `xorriso` - for ISO creation
  - `qemu-system-x86_64` - for testing
  - `ld` (GNU Linker)
- **Optional**: OVMF firmware for UEFI emulation

## Building

Build the kernel with:

```bash
zig build
```

Build and create an ISO image:

```bash
zig build iso
```

## Running

Run the kernel in QEMU:

```bash
zig build run
```

The kernel will boot with SMP enabled and output to the serial console.

## Project Structure

```
pivot-os/
├── src/
│   ├── main.zig           # Kernel entry point
│   ├── drivers/           # Hardware drivers
│   │   ├── acpi.zig       # ACPI support
│   │   ├── cpu.zig        # CPU management
│   │   ├── fb.zig         # Framebuffer
│   │   ├── gdt.zig        # Global Descriptor Table
│   │   ├── idt.zig        # Interrupt Descriptor Table
│   │   ├── lapic.zig      # Local APIC
│   │   ├── ide.zig        # IDE disk driver
│   │   ├── serial.zig     # Serial port
│   │   ├── intctrl/       # Interrupt controllers
│   │   ├── pci/           # PCI and PCIe support
│   │   └── timers/        # Timer drivers (HPET, PIT, TSC)
│   ├── lib/               # Core libraries
│   │   ├── logger.zig     # Logging system
│   │   ├── scheduler.zig  # Task scheduler
│   │   ├── smp.zig        # Symmetric Multi-Processing
│   │   ├── syscalls.zig   # System calls
│   │   └── mem/           # Memory management
│   └── lwip/              # lwIP network stack
├── build.zig              # Build configuration
├── build.zig.zon          # Zig package manifest
├── limine.conf            # Limine bootloader config
├── linker.ld              # Linker script
└── font.sfn               # System font
```

## Task System

The kernel uses a task-based initialization system where drivers and libraries are initialized through a dependency graph. Tasks can depend on other tasks and will automatically execute their dependencies in the correct order.

## Debugging

The kernel includes a logging system with the following levels:
- `debug` - Detailed debug information
- `info` - Informational messages
- `warn` - Warning messages
- `err` - Error messages

Logs are output to the serial console when running in QEMU.

## License

Specify your project license here (e.g., MIT, GPL, etc.)

## Contributing

Contributions are welcome! Please feel free to submit issues or pull requests.

## References

- [Limine Bootloader](https://limine-bootloader.org/)
- [Zig Programming Language](https://ziglang.org/)
- [OSDev Wiki](https://wiki.osdev.org/)
- [Intel SDM](https://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-manual-combined-volumes-1-2a-2b-2c-2d.pdf)
