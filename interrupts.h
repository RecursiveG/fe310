#ifndef __INTERRUPTS_H__
#define __INTERRUPTS_H__

#define PLIC_SOURCE_UART0 3
#define PLIC_SOURCE_UART1 4

void _init_interrupts(void);

inline void set_mstatus_mie(void) __attribute__((always_inline));
inline void set_mstatus_mie(void) {
    __asm__ inline volatile("csrsi mstatus, 8");
}

inline void unset_mstatus_mie(void) __attribute__((always_inline));
inline void unset_mstatus_mie(void) {
    __asm__ inline volatile("csrci mstatus, 8");
}

typedef void (plic_handler_f)(void);
// Returns 0 if success, -1 on error
int plic_handler_register(int source_id, plic_handler_f *handler);
int plic_handler_unregister(int source_id, plic_handler_f *handler);

#endif // __INTERRUPTS_H__
