#include <lib/timer.hpp>
#include <drivers/pit.hpp>
#include <drivers/lapic.hpp>
#include <uacpi/kernel_api.h>
using namespace timer;

uint8_t timer::irq;

void timer::sleep(std::size_t ms) {
    auto& t = lapic::initialized ? lapic::ticks : pit::ticks;
    std::size_t target = t + ms;
    while (t < target) asm ("pause");
}

std::size_t timer::time() {
    return pit::ticks + lapic::ticks;
}