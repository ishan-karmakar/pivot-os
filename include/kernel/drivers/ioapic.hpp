#pragma once
#include <acpi/madt.hpp>
#include <mem/mapper.hpp>

namespace drivers {
    class IOAPIC {
    public:
        enum flags {
            LowestPriority = (1 << 8),
            SMI = (0b10 << 8),
            NMI = (0b100 << 8),
            INIT = (0b101 << 8),
            ExtINT = (0b111 << 8),

            LogicalDest = (1 << 11),

            ActiveLow = (1 << 13),

            LevelTrigger = (1 << 15),

            Masked = (1 << 16)
        };

        IOAPIC() = delete;
        static void init(mem::PTMapper&);
        static void set_irq(uint8_t, uint8_t, uint8_t, uint32_t);
        static void set_mask(uint8_t, bool);

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

        static std::optional<const acpi::MADT::ioapic_so> find_so(uint8_t);

        static uint32_t read_reg(uint32_t);
        static void write_reg(uint32_t, uint32_t);
        static red_ent read_red(uint8_t);
        static void write_red(uint8_t, const red_ent&);

        static uintptr_t addr;
    };
}