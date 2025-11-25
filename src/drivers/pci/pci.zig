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
    pci.read_reg = read_reg;
    pci.write_reg = write_reg;
    pci.scan_devices(0);
    return .success;
}

fn get_addr(addr: uacpi.uacpi_pci_address, off: u13) u32 {
    const bus: u32 = @intCast(addr.bus);
    const device: u32 = @intCast(addr.device);
    const func: u32 = @intCast(addr.function);

    return ((bus << 16) | (device << 11) | (func << 8) | (1 << 31)) + off;
}

pub fn read_reg(addr: uacpi.uacpi_pci_address, off: u13) u32 {
    serial.out(CONFIG_ADDR, get_addr(addr, off));
    return serial.in(CONFIG_DATA, u32);
}

pub fn write_reg(addr: uacpi.uacpi_pci_address, off: u13, val: u32) void {
    serial.out(CONFIG_ADDR, get_addr(addr, off));
    serial.out(CONFIG_DATA, val);
}
