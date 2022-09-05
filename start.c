#include <stdint.h>

#include "linker_symbols.h"
#include "registers.h"

/*********************************************************
 * Set stack pointer, so that we can use local variables *
 * .text.init_stack is put at the beginning of the flash.*
 *********************************************************/
__asm__(
    ".section .text.init_stack\n"
    "init_stack:\n"
    "  la sp, _lds_stack_bottom\n"
    "  j _start"
);

/************************************************************
 * All _start* functions are put in the .text.start section.*
 * They will be executed on SPI flash directly.             *
 ************************************************************/
void _start(void)                                    __attribute__((section(".text.start")));
// This function should only be called after the RTC is initialized.
static void _start_wait_rtc_ticks(int ticks)         __attribute__((section(".text.start")));
// These functions should only be called after .data is copied
// and UART is initialized.
// Until then string constants become valid.
static void _start_putc(char c)                      __attribute__((section(".text.start")));
static void _start_puts(const char* s)               __attribute__((section(".text.start")));
static void _start_hexdump(const void* buf, int len) __attribute__((section(".text.start")));

void _start(void) {
    // Enable RTC: 32.768kHz
    REG(RTCCFG) = 1 << REG_RTCCFG_RTCENALWAYS_SHIFT;

    // Setup PLL
    uint32_t pllr = 1;  // N+1 == 2       refr  = 16MHz / 2 = 8MHz
    uint32_t pllf = 31; // 2*(N+1) == 64  vco   = 8 * 64    = 512MHz
    uint32_t pllq = 3;  // (2^N) == 8     pllout= 512 / 8   = 64MHz
    uint32_t pllcfg =
        ( pllr & 0x7 )        |
        ((pllf & 0x3f) << 4 ) |
        ((pllq & 0x3 ) << 10) |
        ( 1            << 17); // pllrefsel, select HFXOSC
    REG(PRCI_PLLCFG) = pllcfg;
    // Wait for PLL lock
    _start_wait_rtc_ticks(10);
    do {
        int pll_locked = REG(PRCI_PLLCFG) >> 31;
        if (pll_locked) {
            REG(PRCI_PLLCFG) |= 1 << REG_PRCI_PLLCFG_PLLSEL_SHIFT;
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

    // Copy .text contents
    src_ptr = &_lds_text_lma_start;
    dst_ptr = &_lds_text_vma_start;
    while (dst_ptr < &_lds_text_vma_end) {
        *dst_ptr = *src_ptr;
        dst_ptr++;
        src_ptr++;
    }

    // Enable UART0 TX
    REG(GPIO_IOF_EN)  |= 1<<17;
    REG(UART0_TXCTRL)  = 1;
    REG(UART0_DIV)     = 255; // baud rate = 250000
    _start_puts("\r\nUART0 initialized.\r\n");

    // Jump to the main function.
    int main(void);
    main();
    _start_puts("main() function returns. Halting.\r\n");
    while(1);
}

void _start_wait_rtc_ticks(int ticks) {
    uint64_t begin_tick = REG64(CLINT_MTIME);
    while (1) {
        uint64_t now_tick = REG64(CLINT_MTIME);
        if (now_tick - begin_tick > ticks) {
            break;
        }
    }
}

void _start_putc(char c) {
    int full = 1;
    while (full) {
        full = REG(UART0_TXDATA) >> 31;
    }
    REG(UART0_TXDATA) = c;
}

void _start_puts(const char* str) {
    for (const char* ch = str; *ch != '\0'; ch++) {
        _start_putc(*ch);
    }
}

void _start_hexdump(const void* buf, int len) {
    static const char* HEX = "0123456789abcdef";
    while (len --> 0) {
        unsigned char byte = *(const unsigned char*)buf;
        _start_putc(HEX[byte >> 4]);
        _start_putc(HEX[byte & 0xf]);
        buf++;
    }
}
