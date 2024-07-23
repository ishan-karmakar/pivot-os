// #pragma once
// #include <acpi/acpi.hpp>

// namespace acpi {
//     class FADT {
//     public:
//         struct [[gnu::packed]] gas {
//             uint8_t addr_space;
//             uint8_t bit_width;
//             uint8_t bit_off;
//             uint8_t access_size;
//             uint64_t addr;
//         };

//         struct [[gnu::packed]] fadt : public sdt {
//             uint32_t firmware_ctrl;
//             uint32_t dsdt;
//             uint8_t rsv;
//             uint8_t preffered_pmanagement_profile;
//             uint16_t sci_int;
//             uint32_t smi_cmd_port;
//             uint8_t acpi_enable;
//             uint8_t acpi_disable;
//             uint8_t s4bios_req;
//             uint8_t pstate_control;
//             uint32_t pm1aeventblock;
//             uint32_t pm1beventblock;
//             uint32_t pm1acontrolblock;
//             uint32_t pm1bcontrolblock;
//             uint32_t pm2controlblock;
//             uint32_t pmtimerblock;
//             uint32_t gpe0block;
//             uint32_t gpe1block;
//             uint8_t pm1eventlength;
//             uint8_t pm1controllength;
//             uint8_t pm2controllength;
//             uint8_t pmtimerlength;
//             uint8_t gpe0length;
//             uint8_t gpe1length;
//             uint8_t gpe1base;
//             uint8_t cstatecontrol;
//             uint16_t worstc2latency;
//             uint16_t worstc3latency;
//             uint16_t flush_size;
//             uint16_t flush_stride;
//             uint8_t duty_off;
//             uint8_t duty_width;
//             uint8_t day_alarm;
//             uint8_t month_alarm;
//             uint8_t century;

//             uint16_t boot_arch_flags;

//             uint8_t rsv2;
//             uint32_t flags;

//             gas reset_reg;

//             uint8_t reset_val;
//             uint8_t rsv3[3];

//             uint64_t x_firmware_ctrl;
//             uint64_t x_dsdt;

//             gas x_pm1aeventblock;
//             gas x_pm1beventblock;
//             gas x_pm1acontrolblock;
//             gas x_pm1bcontrolblock;
//             gas x_pm2controlblock;
//             gas x_pmtimerblock;
//             gas x_gpe0block;
//             gas x_gpe1block;
//             gas sleep_ctrl_reg;
//             gas sleep_status_reg;
//             uint64_t hv_vendor_identity;
//         };

//         FADT(const sdt *h) : SDT{h}, table{reinterpret_cast<const fadt*>(h)} {};

//         const fadt *table;

//         static constexpr const char *SIGNATURE = "FACP";
//     };
// }
