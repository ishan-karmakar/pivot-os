# PivotOS
The PivotOS technical report is linked [here](https://drive.google.com/file/d/1kxixXEWUIjfGBtqEL4NTH-5zH13HSeFE/view?usp=sharing)

PivotOS is a hobby operating system I developed to learn more about systems programming. The main branch is written in C++ and Assembly, but new work is being implemented in the [Zig Rewrite](../../tree/zig) branch.

# Building
PivotOS supports Clang, and can be built with Meson:
```bash
$ meson setup build --cross x86_64.txt
$ cd build
$ make
```

# Features
Hopefully I will keep this updated, but as of November 2024, PivotOS has implemented the following features:
- [x] UART 16550 Communication (used for QEMU debug port)
- [x] Physical Memory Manager (backed by a linked list allocator)
- [x] Virtual Memory Manager (backed by a buddy allocator implemented with a bitmap)
- [x] Kernel Heap (backed by a slab allocator)
- [x] 8259 PIC (only used to handle PIT interrupts)
- [x] Programmable Interval Timer (only used to calibrate Local APIC Timer)
- [x] Framebuffer (using [flanterm](https://github.com/mintsuki/flanterm))
- [x] Local APIC Timer
- [x] I/O APIC (for routing external interrupts)
- [x] ACPI (using [uACPI](https://github.com/uACPI/uACPI))
- [x] TSS (will be used for userspace)
- [x] Syscalls (only some are implemented)
- [x] SMP (uses [limine](https://github.com/limine-bootloader/limine) to start CPUs)
- [x] Scheduler (Round Robin)
- [x] Virtual File System (only supports regular files and directories)
- [x] TMPFS
- [ ] DEVTMPFS
- [ ] PCI(e)
- [ ] USB
- [ ] MSI
- [ ] MSI-X
- [ ] Userspace
