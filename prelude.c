#include <stdint.h>

#include "registers.h"

/****************************
 * Common library functions *
 ****************************/

int putchar(int c) {
    uint32_t full;
    c &= 0xff;
    do {
        // amoor.w rd, rs1, rs2
        // rs1 holds the memory address.
        // for whatever reason, it needs to be written as
        // amoor.w rd, rs2, (rs1)
        __asm__ volatile("amoor.w %0, %2, (%1)\n"
            : "=r"(full)
            : "r"(REG_UART0_TXDATA), "r"(c)
        );
    } while (full & 0x8000'0000);
    return c;
}

int puts(const char *str) {
    while (*str) putchar(*str++);
    putchar('\r');
    putchar('\n');
    return 1;
}

unsigned int sleep(unsigned int seconds) {
    int64_t timeout_tick = REG64(CLINT_MTIME);
    // 32768 ticks = 1 second
    while (seconds --> 0) timeout_tick += 32768;
    // Busy loop until the time.
    while ((int64_t)REG64(CLINT_MTIME) - timeout_tick < 0);
    return 0;
}

/*************************
 *        Prelude        *
 *************************/

// Prepare runtime for the main function.
void _prelude(void) {
    // Call main(). TODO print return value.
    int main(void);
    // And call it.
    main();
    puts("main() returned");
    while(1) { }
}