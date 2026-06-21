const kernel = @import("root");
const mem = kernel.lib.mem;
const limine = @import("limine");
const std = @import("std");
const log = std.log.scoped(.smbios);

const EntryPoint = extern struct {
    anchor_string: [5]u8,
    entry_point_checksum: u8,
    entry_point_length: u8,
    major_version: u8,
    minor_version: u8,
    docrev: u8,
    entry_point_revision: u8,
    rsv: u8,
    structure_tbl_max_size: u32,
    structure_tbl_addr: u64,
};

const HeaderType = enum(u8) {
    PLATFORM_FIRMWARE_INFORMATION = 0,
    SYSTEM_INFORMATION = 1,
    BASEBOARD_MODULE_INFORMATION = 2,
    SYSTEM_ENCLOSURE_CHASSIS = 3,
    PROCESSOR_INFORMATION = 4,
    CACHE_INFORMATION = 7,
    PORT_CONNECTOR_INFORMATION = 8,
    SYSTEM_SLOTS = 9,
    OEM_STRINGS = 11,
    SYSTEM_CONFIGURATION = 12,
    FIRMWARE_LANGUAGE_INFORMATION = 13,
    GROUP_ASSOCIATIONS = 14,
    SYSTEM_EVENT_LOG = 15,
    PHYSICAL_MEMORY_ARRAY = 16,
    MEMORY_DEVICE = 17,
    MEMORY_ARRAY_MAPPED_ADDRESS = 19,
    MEMORY_DEVICE_MAPPED_ADDRESS = 20,
    BUILTIN_POINTING_DEVICE = 21,
    PORTABLE_BATTERY = 22,
    SYSTEM_RESET = 23,
    HARDWARE_SECURITY = 24,
    SYSTEM_POWER_CONTROLS = 25,
    VOLTAGE_PROBE = 26,
    COOLING_DEVICE = 27,
    TEMPERATURE_PROBE = 28,
    ELECTRICAL_CURRENT_PROBE = 29,
    OUT_OF_BAND_REMOTE_ACCESS = 30,
    BOOT_INTEGRITY_SERVICES = 31,
    SYSTEM_BOOT_INFORMATION = 32,
    MEMORY_ERROR_INFORMATION = 33,
    MANAGEMENT_DEVICE = 34,
    MANAGEMENT_DEVICE_COMPONENT = 35,
    MANAGEMENT_DEVICE_THRESHOLD_DATA = 36,
    MEMORY_CHANNEL = 37,
    IPMI_DEVICE_INFORMATION = 38,
    SYSTEM_POWER_SUPPLY = 39,
    ADDITIONAL_INFORMATION = 40,
    ONBOARD_DEVICES_EXTENDED_INFO = 41,
    MANAGEMENT_CONTROLLER_HOST_INTERFACE = 42,
    TPM_DEVICE = 43,
    PROCESSOR_ADDITIONAL_INFORMATION = 44,
    FIRMWARE_INVENTORY_INFORMATION = 45,
    STRING_PROPERTY = 46,
    END_OF_TABLE = 127,
};

const TableHeader = extern struct {
    type: HeaderType,
    length: u8,
    handle: u16 align(1),

    pub fn from(table: anytype) *const @This() {
        return @ptrFromInt(@intFromPtr(table) - @sizeOf(@This()));
    }

    pub fn to(self: *const @This(), TableType: type) *const TableType {
        return @ptrFromInt(@intFromPtr(self) + @sizeOf(@This()));
    }
};

const SystemInformation = extern struct {
    pub const WakeUpType = enum(u8) {
        OTHER = 1,
        UNKNOWN = 2,
        APM_TIMER = 3,
        MODERN_RING = 4,
        LAN_REMOTE = 5,
        POWER_SWITCH = 6,
        PCI_PME = 7,
        AC_POWER_RESTORED = 8,
    };

    manufacturer: u8,
    product_name: u8,
    version: u8,
    serial_number: u8,
    UUID: [16]u8,
    wake_up_type: WakeUpType,
    sku_number: u8,
    family: u8,
};

const SystemEnclosureChassis = packed struct {
    pub const ChassisType = enum(u7) {
        OTHER = 1,
        UNKNOWN = 2,
        DESKTOP = 3,
        LOW_PROFILE_DESKTOP = 4,
        PIZZA_BOX = 5,
        MINI_TOWER = 6,
        TOWER = 7,
        PORTABLE = 8,
        LAPTOP = 9,
        NOTEBOOK = 0xA,
        HAND_HELD = 0xB,
        DOCKING_STATION = 0xC,
        ALL_IN_ONE = 0xD,
        SUB_NOTEBOOK = 0xE,
        SPACE_SAVING = 0xF,
        LUNCH_BOX = 0x10,
        MAIN_SERVER_CHASSIS = 0x11,
        EXPANSION_CHASSIS = 0x12,
        SUB_CHASSIS = 0x13,
        BUS_EXPANSION_CHASSIS = 0x14,
        PERIPHERAL_CHASSIS = 0x15,
        RAID_CHASSIS = 0x16,
        RACK_MOUNT_CHASSIS = 0x17,
        SEALED_CASE_PC = 0x18,
        MULTI_SYSTEM_CHASSIS = 0x19,
        COMPACT_PCI = 0x1A,
        ADVANCED_TCA = 0x1B,
        BLADE = 0x1C,
        BLADE_ENCLOSURE = 0x1D,
        TABLET = 0x1E,
        CONVERTIBLE = 0x1F,
        DETACHABLE = 0x20,
        IOT_GATEWAY = 0x21,
        EMBEDDED_PC = 0x22,
        MINI_PC = 0x23,
        STICK_PC = 0x24,
    };

    pub const State = enum(u8) {
        OTHER = 1,
        UNKNOWN = 2,
        SAFE = 3,
        WARNING = 4,
        CRITICAL = 5,
        NON_RECOVERABLE = 6,
    };

    pub const SecurityStatus = enum(u8) {
        OTHER = 1,
        UNKNOWN = 2,
        NONE = 3,
        EXTERNAL_INTERFACE_LOCKED_OUT = 4,
        EXTERNAL_INTERFACE_ENABLED = 5,
    };

    manufacturer: u8,
    type: ChassisType,
    chassis_lock: bool,
    version: u8,
    serial_number: u8,
    asset_tag_number: u8,
    boot_up_state: State,
    power_supply_state: State,
    thermal_state: State,
    security_status: SecurityStatus,
    oem: u32,
    height: u8,
    num_power_cords: u8,
};

export var SMBIOS_REQUEST = limine.limine_smbios_request{
    .id = kernel.LIMINE_REQUEST_ID(0x9e9046f11e095391, 0xaa4a520fefbde5ee),
};

var initialized = false;

pub fn init() !void {
    if (initialized)
        return kernel.lib.logger.already_initialized(log, "SMBIOS");
    mem.pmm.init() catch |err|
        return kernel.lib.logger.failed_initialization(log, "SMBIOS", err);

    if (SMBIOS_REQUEST.response == null or SMBIOS_REQUEST.response.*.entry_64 == null)
        return kernel.lib.logger.failed_initialization(log, "SMBIOS", error.SMBIOSUnavailable);

    const entry_point: *const EntryPoint = @ptrCast(@alignCast(SMBIOS_REQUEST.response.*.entry_64));
    log.debug("Version: {}.{}.{}, entry point revision: {}", .{ entry_point.major_version, entry_point.minor_version, entry_point.docrev, entry_point.entry_point_revision });

    var cur_table: *const TableHeader = @ptrFromInt(mem.virt(entry_point.structure_tbl_addr));
    while (true) {
        switch (cur_table.type) {
            .SYSTEM_INFORMATION => parse_system_information(cur_table.to(SystemInformation)),
            .SYSTEM_ENCLOSURE_CHASSIS => parse_system_enclosure_chassis(cur_table.to(SystemEnclosureChassis)),
            .END_OF_TABLE => break,
            else => log.debug("Unknown table type: {s}", .{@tagName(cur_table.type)}),
        }
        cur_table = next_table(cur_table);
    }

    initialized = true;
    return kernel.lib.logger.successfully_initialized(log, "SMBIOS");
}

fn parse_system_information(table: *const SystemInformation) void {
    log.debug("System Information Table:", .{});
    log.debug("-> Manufacturer: {s}", .{get_string(table, table.manufacturer)});
    log.debug("-> Product Name: {s}", .{get_string(table, table.product_name)});
    log.debug("-> Version: {s}", .{get_string(table, table.version)});
    log.debug("-> Serial Number: {s}", .{get_string(table, table.serial_number)});
    log.debug("-> UUID: {any}", .{table.UUID});
    log.debug("-> Wake Up Type: {s}", .{@tagName(table.wake_up_type)});
    log.debug("-> SKU Number: {s}", .{get_string(table, table.sku_number)});
    log.debug("-> Family: {s}", .{get_string(table, table.family)});
    log.debug("", .{});
}

fn parse_system_enclosure_chassis(table: *const SystemEnclosureChassis) void {
    log.debug("System Enclosure or Chassis:", .{});
    log.debug("-> Chassis Type: {s}, Chassis Lock: {}", .{ @tagName(table.type), table.chassis_lock });
    log.debug("-> Version: {s}", .{get_string(table, table.version)});
    log.debug("-> Serial Number: {s}", .{get_string(table, table.serial_number)});
    log.debug("-> Asset Tag Number: {s}", .{get_string(table, table.asset_tag_number)});
    log.debug("-> Boot-up State: {s}", .{@tagName(table.boot_up_state)});
    log.debug("-> Power Supply State: {s}", .{@tagName(table.power_supply_state)});
    log.debug("-> Thermal State: {s}", .{@tagName(table.thermal_state)});
    log.debug("-> Security Status: {s}", .{@tagName(table.security_status)});
    log.debug("-> OEM Data: {}", .{table.oem});
    log.debug("-> Height: {} U", .{table.height});
    log.debug("-> Number of Power Cords: {}", .{table.num_power_cords});
    log.debug("", .{});
}

fn next_table(table: *const TableHeader) *const TableHeader {
    const bytes: [*]const u8 = @ptrCast(table);
    const strtab = bytes[table.length..];
    var i: usize = 0;
    while (true) : (i += 1) {
        if (strtab[i] == 0 and strtab[i + 1] == 0) {
            i += 2;
            break;
        }
    }
    return @ptrFromInt(@intFromPtr(table) + table.length + i);
}

fn get_string(table: anytype, idx: u8) []const u8 {
    if (idx == 0) return "";
    const header = TableHeader.from(table);
    const bytes: [*]const u8 = @ptrCast(header);
    const strtab: [*:0]const u8 = @ptrCast(bytes[header.length..]);
    var cur: usize = 0;
    for (1..idx) |_|
        cur += std.mem.span(strtab[cur..]).len + 1;

    return std.mem.span(strtab[cur..]);
}
