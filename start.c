#include <stdint.h>
#include "linker_symbols.h"

__asm__(
    ".section .text.bootloader\n"
    ".global init_stack\n"
    "init_stack:\n"
    "  la sp, _lds_stack_bottom\n"
    "  j _start"
);

// 32768 ticks = 1 second
void sleep(int ticks) {
    volatile uint64_t* const REG_CLINT_MTIME = (uint64_t*)0x0200bff8;
    uint64_t begin_tick = *REG_CLINT_MTIME;
    while (1) {
        uint64_t end_tick = *REG_CLINT_MTIME;
        if (end_tick - begin_tick > ticks) {
            return;
        }
    }
}

int main(void);
void _start(void) __attribute__((section(".text.start")));
void _start(void) {
    // Enable RTC: 32.768kHz
    volatile uint32_t* const REG_RTCCFG = (uint32_t*)0x10000040;
    *REG_RTCCFG = 1 << 12;

    // Setup PLL
    volatile uint32_t* const REG_PRCI_PLLCFG = (uint32_t*)0x1000'8008;
    uint32_t pllr = 1;  // N+1 == 2       refr  = 16MHz / 2 = 8MHz
    uint32_t pllf = 31; // 2*(N+1) == 64  vco   = 8 * 64    = 512MHz
    uint32_t pllq = 3;  // (2^N) == 8     pllout= 512 / 8   = 64MHz
    uint32_t pllcfg =
        ( pllr & 0x7 )        |
        ((pllf & 0x3f) << 4 ) |
        ((pllq & 0x3 ) << 10) |
        ( 1            << 17); // pllrefsel, select HFXOSC
    *REG_PRCI_PLLCFG = pllcfg;
    // Wait for PLL lock
    volatile uint64_t* const REG_CLINT_MTIME = (uint64_t*)0x0200bff8;
    uint64_t begin_tick = *REG_CLINT_MTIME;
    while (1) {
        uint64_t now_tick = *REG_CLINT_MTIME;
        if (now_tick - begin_tick > 10) {
            break;
        }
    }
    do {
        int pll_locked = (*REG_PRCI_PLLCFG) >> 31;
        if (pll_locked) {
            *REG_PRCI_PLLCFG |= 1 << 16;  // pllsel
            break;
        }
    } while (1);
    
    // Zero-fill .bss
    for (uint8_t *ptr = &_lds_bss_start; ptr < &_lds_bss_end; ptr++) {
        *ptr = 0;
    }

    // Copy .data contents
    volatile uint8_t* src_ptr = &_lds_data_lma_start;
    volatile uint8_t* dst_ptr = &_lds_data_vma_start;
    while (dst_ptr < &_lds_data_vma_end) {
        *dst_ptr = *src_ptr;
        dst_ptr++;
        src_ptr++;
    }

    // Enable UART0 TX
    volatile uint32_t* const REG_GPIO_IOF_EN = (uint32_t*)0x10012038;
    volatile uint32_t* const REG_UART0_TXCTRL = (uint32_t*)0x10013008;
    volatile uint32_t* const REG_UART0_DIV = (uint32_t*)0x10013018;
    *REG_GPIO_IOF_EN |= 1 << 17;
    *REG_UART0_TXCTRL = 1;
    *REG_UART0_DIV = 255; // baud rate = 250000

    // Jump to main function
    main();
}

void uart0_write(const char* str) {
    volatile uint32_t* const REG_UART0_TXDATA = (uint32_t*)0x10013000;
    for (const char* ch = str; *ch != '\0'; ch++) {
        int full = 1;
        while (full) {
            full = (*REG_UART0_TXDATA) >> 31;
        }
        *REG_UART0_TXDATA = *ch;
    }
}

int main(void) {
    uart0_write("hello risc-v\r\n");
    int led_on = 1;
    volatile uint32_t* const REG_GPIO_IOF_EN = (uint32_t*)0x10012038;
    volatile uint32_t* const REG_GPIO_INPUT_EN = (uint32_t*)0x10012004;
    volatile uint32_t* const REG_GPIO_OUTPUT_EN = (uint32_t*)0x10012008;
    volatile uint32_t* const REG_GPIO_OUTPUT_VAL = (uint32_t*)0x1001200C;
    
    // *REG_GPIO_IOF_EN = 0;
    *REG_GPIO_OUTPUT_EN |= 0x20;
    while(1) {
        if (led_on) {
            uart0_write("led_off\r\n");
            *REG_GPIO_OUTPUT_VAL &= 0xFFFFFFDFU;
            led_on = 0;
        } else {
            uart0_write("led_on\r\n");
            *REG_GPIO_OUTPUT_VAL |= 0x20U;
            led_on = 1;
        }
        sleep(32768);
    }
}