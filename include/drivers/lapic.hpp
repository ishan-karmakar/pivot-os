#pragma once
#include <cstdint>

namespace mem {
    class PTMapper;
}

namespace idt {
    class IDT;
}

namespace drivers {

    class LAPIC {
    public:
        LAPIC() = delete;

        // Init for BSP processor - Completes full initialization of APIC
        static void init(idt::IDT&);

        // Init for AP processors - Only enables APIC, as initialization is already done by BSP
        static void init();

        static void calibrate();
        static inline void eoi() { write_reg(0xB0, 0); }

        static uint32_t ms_interval;

    private:
        static void write_reg(uint32_t, uint64_t);
        static uint64_t read_reg(uint32_t);

        static bool x2mode;
        static uintptr_t lapic;
        static bool initialized;

        static constexpr int SPURIOUS_IDT_ENT = 0xFF;
        static constexpr int PERIODIC_IDT_ENT = 32;

        static constexpr int IA32_APIC_BASE = 0x1B;

        static constexpr int TDIV1 = 0xB;
        static constexpr int TDIV2 = 0;
        static constexpr int TDIV4 = 1;
        static constexpr int TDIV8 = 2;
        static constexpr int TDIV16 = 3;
        static constexpr int TDIV32 = 8;
        static constexpr int TDIV64 = 9;
        static constexpr int TDIV128 = 0xA;

        static constexpr int TDIV = TDIV4;

        static constexpr int SPURIOUS_OFF = 0xF0;
        static constexpr int INITIAL_COUNT_OFF = 0x380;
        static constexpr int CUR_COUNT_OFF = 0x390;
        static constexpr int CONFIG_OFF = 0x3E0;
    };
}
