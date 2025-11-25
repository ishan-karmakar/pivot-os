const uacpi = @import("uacpi");
const kernel = @import("root");
const serial = kernel.drivers.serial;
const pci = kernel.drivers.pci;

const CONFIG_ADDR = 0xCF8;
const CONFIG_DATA = 0xCFC;

pub var Task = kernel.Task{
    .name = "PCI",
    .init = init,
    .dependencies = &.{},
};

fn init() kernel.Task.Ret {
    pci.read_reg8 = GenerateReadFunc(u8);
    pci.read_reg16 = GenerateReadFunc(u16);
    pci.read_reg32 = GenerateReadFunc(u32);
    pci.write_reg8 = GenerateWriteFunc(u8);
    pci.write_reg16 = GenerateWriteFunc(u16);
    pci.write_reg32 = GenerateWriteFunc(u32);
    pci.scan_devices(0);
    return .success;
}

fn get_addr(addr: uacpi.uacpi_pci_address, off: u13) u32 {
    const bus: u32 = @intCast(addr.bus);
    const device: u32 = @intCast(addr.device);
    const func: u32 = @intCast(addr.function);

    return ((bus << 16) | (device << 11) | (func << 8) | (1 << 31)) + off;
}

fn GenerateReadFunc(T: type) pci.ReadFunc(T) {
    return struct {
        fn read_reg(addr: uacpi.uacpi_pci_address, off: u13) T {
            serial.out(CONFIG_ADDR, get_addr(addr, off));
            return serial.in(CONFIG_DATA, T);
        }
    }.read_reg;
}

fn GenerateWriteFunc(T: type) pci.WriteFunc(T) {
    return struct {
        fn write_reg(addr: uacpi.uacpi_pci_address, off: u13, val: T) void {
            serial.out(CONFIG_ADDR, get_addr(addr, off));
            return serial.out(CONFIG_DATA, val);
        }
    }.write_reg;
}
