#include <lib/timer.hpp>
#include <drivers/pit.hpp>
#include <drivers/lapic.hpp>
#include <uacpi/kernel_api.h>
#include <cpu/smp.hpp>
#include <kernel.hpp>

uint8_t timer::irq;

namespace timer {
    void sleep(std::size_t ms) {
        auto& t = lapic::initialized ? lapic::ticks : pit::ticks;
        std::size_t target = t + ms;
        while (t < target) asm ("pause");
    }

    std::size_t time() {
        return pit::ticks + lapic::ticks;
    }
}

void uacpi_kernel_sleep(std::size_t ms) { timer::sleep(ms); }
std::size_t uacpi_kernel_get_ticks() { return timer::time(); }
