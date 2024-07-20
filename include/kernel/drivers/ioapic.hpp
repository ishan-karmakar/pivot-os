#pragma once
#include <acpi/madt.hpp>
#include <optional>

namespace mem {
    class PTMapper;
}

namespace drivers {
    class IOAPIC {
    public:
        IOAPIC() = delete;
        static void init(mem::PTMapper&);
        static void set_irq(uint8_t, uint8_t, uint8_t, uint32_t);
        static void set_mask(uint8_t, bool);
        
        static constexpr int LOWEST_PRIORITY = (1 << 8);
        static constexpr int SMI = (0b10 << 8);
        static constexpr int NMI = (0b100 << 8);
        static constexpr int INIT = (0b101 << 8);
        static constexpr int EXT_INT = (0b111 << 8);
        static constexpr int LOGICAL_DEST = (1 << 11);
        static constexpr int ACTIVE_LOW = (1 << 13);
        static constexpr int LEVEL_TRIGGER = (1 << 15);
        static constexpr int MASKED = (1 << 16);

    private:
        union red_ent {
            struct {
                uint8_t vector;
                uint8_t delivery_mode:3;
                uint8_t dest_mode:1;
                uint8_t delivery_status:1;
                uint8_t pin_polarity:1;
                uint8_t remote_irr:1;
                uint8_t trigger_mode:1;
                uint8_t mask:1;
                uint64_t rsv:39;
                uint8_t dest;
            };
            uint64_t raw;
        };

        static std::optional<acpi::MADT::ioapic_so> find_so(uint8_t);

        static uint32_t read_reg(uint32_t);
        static void write_reg(uint32_t, uint32_t);
        static red_ent read_red(uint8_t);
        static void write_red(uint8_t, const red_ent&);

        static uintptr_t addr;

        static constexpr int VERSION_OFF = 1;
    };
}