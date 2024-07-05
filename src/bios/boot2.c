void printc(char);
void print(const char*);

void _start() {
    print("Hello World\r\n");
    while(1);
}

void print(const char *str) {
    for (; *str; str++)
        printc(*str);
}

void printc(char c) {
    asm (
        "mov %0, %%al\n"
        "mov $0xE, %%ah\n"
        "mov $0, %%bh\n"
        "mov $0xF, %%bl\n"
        "int $0x10\n"
        : : "g" (c) : "ah", "al", "bh", "bl"
    );
}