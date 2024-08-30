#pragma once
#include <cstdint>
#include <io/serial.hpp>

namespace pci {
    void init();

    // TODO: Support PCIe ECAM
    template <typename T>
    T read(uint8_t bus, uint8_t dev, uint8_t func, uint8_t off) {
        io::out<uint32_t>(0xCF8, (bus << 16) | (dev << 11) | (func << 8) | (off & 0xFC) | 0x80000000U);
        return io::in<T>(0xCFC);
    }

    template <typename T>
    void write(uint8_t bus, uint8_t dev, uint8_t func, uint8_t off, T val) {
        io::out<uint32_t>(0xCF8, (bus << 16) | (dev << 11) | (func << 8) | (off & 0xFC) | 0x80000000U);
        io::out<T>(0xCFC + (off & 3), val);
    }
}