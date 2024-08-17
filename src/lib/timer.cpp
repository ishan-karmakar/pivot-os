#include <lib/timer.hpp>
#include <drivers/pit.hpp>
#include <uacpi/kernel_api.h>
using namespace timer;

void timer::sleep(std::size_t ms) {
    std::size_t target = pit::ticks + ms;
    while (pit::ticks < target) asm ("pause");
}

std::size_t timer::ticks() {
    return pit::ticks;
}

extern "C" void uacpi_kernel_stall(uacpi_u8) {
    // us can be max of 256 so we can just sleep for 1 millisecond
    uacpi_kernel_sleep(1);
}

extern "C" void uacpi_kernel_sleep(uacpi_u64 ms) {
    timer::sleep(ms);
}

uacpi_u64 uacpi_kernel_get_ticks() {
    return timer::ticks();
}