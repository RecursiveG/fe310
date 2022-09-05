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

/**************
 * User codes *
 **************/

int main(void) {
    puts("Hello RISC-V!");

    int led_on = 0;
    REG(GPIO_OUTPUT_EN) |= 0x20;
    while (1) {
        if (led_on) {
            puts("led_off");
            REG(GPIO_OUTPUT_VAL) &= 0xFFFFFFDFU;
            led_on = 0;
        } else {
            puts("led_on");
            REG(GPIO_OUTPUT_VAL) |= 0x20U;
            led_on = 1;
        }
        sleep(1);
    }
}
