#include <lib/timer.hpp>
#include <drivers/pit.hpp>
#include <drivers/lapic.hpp>
#include <uacpi/kernel_api.h>
#include <cpu/smp.hpp>
#include <kernel.hpp>
using namespace timer;

uint8_t timer::irq;

ALIAS_FN(uacpi_kernel_sleep)
[[gnu::noinline]] void timer::sleep(std::size_t ms) {
    auto& t = lapic::initialized ? lapic::ticks : pit::ticks;
    std::size_t target = t + ms;
    while (t < target) asm ("pause");
}

ALIAS_FN(uacpi_kernel_get_ticks)
[[gnu::noinline]] std::size_t timer::time() {
    return pit::ticks + lapic::ticks;
}