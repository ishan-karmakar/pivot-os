#pragma once
#include <stdint.h>

#define KEYBOARD_PORT 0x60
#define KEYBOARD_ACK 0xFA
#define PS2_DATA 0x60
#define PS2_STATUS 0x64
#define PS2_COMMAND 0x64
#define KEY_RELEASE_MASK 0x80

typedef enum {
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
} key_code_t;

typedef enum {
    left_shift = (1<<0),
    right_shift = (1 << 1),

    left_alt = (1 << 2),
    right_alt = (1 << 3),

    left_ctrl = (1 << 4),
    right_ctrl = (1 << 5),
    
    left_gui = (1 << 6),
    right_gui = (1 << 7),

} key_modifiers_t;

typedef struct {
    key_code_t code;
    uint8_t modifiers;
} key_status_t;

char keymap[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '/', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '\\', 0,
    '*', 0, ' ', 0,
    0, 0 ,0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.'
};
char shift_keymap[] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '?', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '|', 0,
    '*', 0, ' ', 0,
    0, 0 ,0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.'
};

void init_keyboard(void);
void handle_keyboard(void);
