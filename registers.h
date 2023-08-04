#ifndef __REGISTERS_H
#define __REGISTERS_H

#include <stdint.h>

#define BIT(x) (1u << (x))

#define REG(name)   (*(volatile uint32_t*)(REG_##name))
#define REG64(name) (*(volatile uint64_t*)(REG_##name))

#define AREG(name)       ((volatile uint32_t*)(AREG_##name))
#define AREG64(name)     ((volatile uint64_t*)(AREG_##name))

#define REG_CLINT_MSIP      0x0200'0000u
#define REG_CLINT_MTIMECMP  0x0200'4000u
#define REG_CLINT_MTIME     0x0200'bff8u
#define REG_RTCCFG          0x1000'0040u
#define REG_PRCI_PLLCFG     0x1000'8008u
#define REG_GPIO_OUTPUT_EN  0x1001'2008u
#define REG_GPIO_OUTPUT_VAL 0x1001'200cu
#define REG_GPIO_IOF_EN     0x1001'2038u
#define REG_UART0_TXDATA    0x1001'3000u
#define REG_UART0_RXDATA    0x1001'3004u
#define REG_UART0_TXCTRL    0x1001'3008u
#define REG_UART0_RXCTRL    0x1001'300cu
#define REG_UART0_IE        0x1001'3010u
#define REG_UART0_DIV       0x1001'3018u
#define REG_PLIC_M_PRIORITY_THRESHOLD   0x0c20'0000u
#define REG_PLIC_M_CLAIM_COMPLETION     0x0c20'0004u

#define AREG_PLIC_PRIORITY      0x0c00'0000u
#define AREG_PLIC_PENDING       0x0c00'1000u
#define AREG_PLIC_M_ENABLE      0x0c00'2000u


#define REG_RTCCFG_RTCENALWAYS_SHIFT 12
#define REG_PRCI_PLLCFG_PLLSEL_SHIFT 16

#endif //__REGISTERS_H
