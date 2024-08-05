#pragma once
#include <cstdint>

namespace cpu {
    struct cpu_status;
    extern "C" cpu::cpu_status *keyboard_handler(cpu::cpu_status*);
}

namespace idt {
    class IDT;
}

namespace drivers {
    class Keyboard {
    public:
        Keyboard() = delete;
        static void init(idt::IDT&);
    
    private:
        enum code {
            // This enum contain the value of the kernel scancodes
            //Row 1
            NO_KEY = 0,
            ESCAPE = 0x01,
            D1 = 0x02,
            D2 = 0x03,    
            D3 = 0x04,
            D4 = 0x05,
            D5 = 0x06,
            D6 = 0x07,
            D7 = 0x08,
            D8 = 0x09,
            D9 = 0x0A,
            D0 = 0x0B,
            MINUS = 0x0C,
            EQUALS = 0x0D,
            BACKSPACE = 0x0E,
            TAB = 0x0F,
            Q = 0x10,
            W = 0x11,
            E = 0x12,
            R = 0x13,
            T = 0x14,
            Y = 0x15,
            U = 0x16,
            I = 0x17,
            O = 0x18,
            P = 0x19,
            SQBRACKET_OPEN = 0x1A,
            SQBRACKET_CLOSE = 0x1B,
            ENTER = 0x1C,
            LEFT_CTRL = 0x1D,
            A = 0x1E,
            S = 0x1F,
            D = 0x20,
            F = 0x21,
            G = 0x22,
            H = 0x23,
            J = 0x24,
            K = 0x25,
            L = 0x26,
            SEMICOLON = 0x27,
            SINGLE_QUOTE = 0x28,
            BACK_TICK = 0x29,
            LEFT_SHIFT = 0x2A,
            SLASH = 0x2B,
            Z = 0x2C,
            X = 0x2D,
            C = 0x2E,
            V = 0x2F,
            B = 0x30,
            N = 0x31,
            M = 0x32,
            COMMA = 0x33,
            DOT = 0x34,
            BACKSLASH = 0x35,
            RIGHT_SHIFT = 0x36,
            KEYPAD_STAR = 0x37,
            LEFT_ALT = 0x38,
            SPACE = 0x39,
            CAPS_LOCK = 0x3A,
            F1 = 0x3B,
            F2 = 0x3C,
            F3 = 0x3D,
            F4 = 0x3E,
            F5 = 0x3F,
            F6 = 0x40,
            F7 = 0x41,
            F8 = 0x42,
            F9 = 0x43,
            F10 = 0x44,
            NUM_LOCK = 0x45,
            SCROLL_LOCK = 0x46,
            KEYPAD_D7 = 0x47,
            KEYPAD_D8 = 0x48,
            KEYPAD_D9 = 0x49,
            KEYPAD_MINUS = 0x4A,
            KEYPAD_D4 = 0x4B,
            KEYPAD_D5 = 0x4C,
            KEYPAD_D6 = 0x4D,
            KEYPAD_PLUS = 0x4E,
            KEYPAD_D1 = 0x4F,
            KEYPAD_D2 = 0x50,
            KEYPAD_D3 = 0x51,
            KEYPAD_D0 = 0x52,
            KEYPAD_DOT = 0x53,
            ALT_SYSRQ = 0x54,
            F11 = 0x57,
            F12 = 0x58,
            LEFT_GUI = 0x5B,
            RIGHT_GUI = 0x5C,
            EXTENDED_PREFIX = 0xE0
        };

        static code translate(uint8_t);
        static void update_modifiers(bool);
        static void check_ack();

        friend cpu::cpu_status *cpu::keyboard_handler(cpu::cpu_status*);

        static constexpr int PORT = 0x60;
        static constexpr int STATUS = 0x64;

        static constexpr int IDT_ENT = 35;
        static constexpr int IRQ_ENT = 1;
    };
}