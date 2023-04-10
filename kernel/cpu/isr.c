#include "isr.h"
#include "log.h"

void exception_handler(void) {
    log(Error, "ISR", "Received exception");
}
