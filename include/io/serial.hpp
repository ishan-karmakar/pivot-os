#pragma once
#include <concepts>
#include <cstdint>
#include <string>

namespace io {
    template <typename T>
    inline void out(int port, T data) {
        if constexpr (std::same_as<T, uint8_t>)
            asm volatile ("outb %b0,%w1" : :"a" (data), "Nd" (port));
        else if constexpr (std::same_as<T, uint16_t>)
            asm volatile ("outw %w0,%w1" :: "a" (data), "Nd" (port));
        else if constexpr (std::same_as<T, uint32_t>)
            asm volatile ("outl %0,%w1" :: "a" (data), "Nd" (port));
    }

    template <typename T>
    inline T in(int port) {
        T data;
        if constexpr (std::same_as<T, uint8_t>)
            asm volatile ("inb %w1,%0":"=a" (data):"Nd" (port));
        else if constexpr (std::same_as<T, uint16_t>)
            asm volatile ("inw %w1,%0" : "=a" (data) : "Nd" (port));
        else if constexpr (std::same_as<T, uint32_t>)
            asm volatile ("inl %w1, %0" : "=a" (data) : "Nd" (port));
        return data;
    }

    class serial_port {
    public:
        serial_port(uint16_t);
        void append(char);
        void append(std::string_view s) { for (char c : s) append(c); }
        void append(const char *s) { append(std::string_view{s}); }

    private:
        uint16_t port;
    };
}