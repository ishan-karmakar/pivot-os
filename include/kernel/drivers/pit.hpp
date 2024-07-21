#pragma once

namespace cpu {
    class IDT;
}

namespace drivers {
    class PIT {
    public:
        PIT() = delete;
        ~PIT() = delete;

        static void init(cpu::IDT&);
        static void cmd(bool, uint8_t, uint8_t, uint8_t);
        static void data(uint16_t);

        static volatile size_t ticks;
        static constexpr int MS_TICKS = 1193;

    private:
        static constexpr int IRQ_ENT = 0;
        static constexpr int IDT_ENT = 34;
        static constexpr int CMD_REG = 0x43;
        static constexpr int DATA_REG = 0x40;
    };
}