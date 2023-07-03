#include <stdint.h>

#include "registers.h"
#include "prelude.h"

static void __attribute__((interrupt, aligned(64))) interrupt_handler(void) {
  // Attribute interrupt does the stack setup & mret for us. Nice!
  puts("interrupt!");
  uint32_t mcause;
  __asm__ volatile("csrr %0, mcause":"=r"(mcause));
  mcause &= 0x8000'03ff;
  if (mcause == 0x8000'0007) {
    // counter wraparound after 1.7e7 years. no need to worry.
    REG64(CLINT_MTIMECMP) += 16384;
  }
}

void _init_interrupts(void) {
  uint32_t tmp = 0;
  uint32_t handler = (uint32_t)interrupt_handler;
  handler &= 0xffff'fffc; // unset the lower 2 bits
  // Setup handler address
  __asm__ volatile("csrw mtvec, %0" ::"r"(handler));
  // Enable timer interrupt
  tmp = 0x80;
  __asm__ volatile("csrs mie, %0"::"r"(tmp));
  // Set timer timeout
  REG64(CLINT_MTIMECMP) = 16384; // interrupt every half second.
  // Enable interrupt
  __asm__ volatile("csrsi mstatus, 8");
}
