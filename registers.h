#ifndef __REGISTERS_H
#define __REGISTERS_H

#include <stdint.h>

#define REG(name)   (*(volatile uint32_t*)(REG_##name))
#define REG64(name) (*(volatile uint64_t*)(REG_##name))

#define REG_CLINT_MTIMECMP  0x0200'4000u
#define REG_CLINT_MTIME     0x0200'bff8u
#define REG_RTCCFG          0x1000'0040u
#define REG_PRCI_PLLCFG     0x1000'8008u
#define REG_GPIO_OUTPUT_EN  0x1001'2008u
#define REG_GPIO_OUTPUT_VAL 0x1001'200cu
#define REG_GPIO_IOF_EN     0x1001'2038u
#define REG_UART0_TXDATA    0x1001'3000u
#define REG_UART0_TXCTRL    0x1001'3008u
#define REG_UART0_DIV       0x1001'3018u

#define REG_RTCCFG_RTCENALWAYS_SHIFT 12
#define REG_PRCI_PLLCFG_PLLSEL_SHIFT 16

#endif //__REGISTERS_H
