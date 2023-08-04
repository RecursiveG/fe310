#include <stdint.h>

#include "interrupts.h"
#include "prelude.h"
#include "registers.h"

#define MI_SOFTWARE  3
#define MI_TIMER     7
#define MI_EXTERNAL  11

typedef void(mi_handler_f)(void);
static void do_nothing_handler(){};

static mi_handler_f *mi_software_handler;
static mi_handler_f *mi_timer_handler;
static mi_handler_f *mi_external_handler;

static void __attribute__((interrupt, aligned(64))) interrupt_handler(void) {
// Attribute interrupt does the stack setup & mret for us. Nice!

    uint32_t mcause;
    __asm__ volatile("csrr %0, mcause" : "=r"(mcause));
    // Interrupt or Exception?
    int is_interrupt = mcause >> 31;
    int exception_code = mcause & 0x3ff; // low 10 bits.

    if (is_interrupt) {
        if (exception_code == MI_TIMER) {
            mi_timer_handler();
        } else if (exception_code == MI_EXTERNAL) {
            mi_external_handler();
        } else if (exception_code == MI_SOFTWARE) {
            mi_software_handler();
        } else {
            fatal("unknown interrupt: %d", exception_code);
        }
        return;
    }

    // exceptions
    switch (exception_code) {
        case 0: halt("instruction address misaligned");
        case 1: halt("instruction access fault");
        case 2: halt("illegal instruction");
        case 3: halt("breakpoint");
        case 4: halt("load address misaligned");
        case 5: halt("load access fault");
        case 6: halt("store/AMO address misaligned");
        case 7: halt("store/AMO access fault");
        case 8: halt("environment call from U-mode");
        case 11: halt("environment call from M-mode");
        default: fatal("unknown exception: %d", exception_code);
    }
}

static void handle_software_interrupt(void) {
    REG(CLINT_MSIP) = 0;
    puts("Received software interrupt.");
}
static void init_msi(void) {
    mi_software_handler = &handle_software_interrupt;
    const uint32_t mie_setval = BIT(MI_SOFTWARE);
    __asm__ volatile("csrs mie, %0" ::"r"(mie_setval));
}

// Timer interrupts every 0.5 second.
#define MI_TIMER_PERIOD 16384
static void handle_timer_interrupt(void) {
    // puts("timer interrupt!");
    // counter wraparound after 1.7e7 years. no need to worry.
    REG64(CLINT_MTIMECMP) += MI_TIMER_PERIOD;
}
static void init_mti(void) {
    // The first interrupt will fire after one period.
    REG64(CLINT_MTIMECMP) = REG64(CLINT_MTIME) + MI_TIMER_PERIOD;
    mi_timer_handler = &handle_timer_interrupt;
    const uint32_t mie_setval = BIT(MI_TIMER);
    __asm__ volatile("csrs mie, %0" ::"r"(mie_setval));
}


// Valid source ids are [1, 52]
#define PLIC_MAX_INTERRUPT 52
static plic_handler_f* plic_handlers[PLIC_MAX_INTERRUPT]; // note that array index is [0, 51]
static void handle_plic_interrupt(void) {
    // Claim
    uint32_t source_id = REG(PLIC_M_CLAIM_COMPLETION);
    if (source_id == 0) halt("phantom plic interrupt");

    while (source_id != 0) {
        if (source_id > PLIC_MAX_INTERRUPT)
            fatal("invalid PLIC source id %d", source_id);

        // Processing
        plic_handler_f *handler = plic_handlers[source_id-1];
        if (handler == NULL)
            fatal("missing handler for PLIC source %d", source_id);
        handler();

        // Completion
        REG(PLIC_M_CLAIM_COMPLETION) = source_id;

        // Try claim another.
        source_id = REG(PLIC_M_CLAIM_COMPLETION);
    }
}
static void init_mei(void) {
    // Enables all priorities except 0
    REG(PLIC_M_PRIORITY_THRESHOLD) = 0;

    for (int i = 1; i <= PLIC_MAX_INTERRUPT; ++i) {
        // Defaults priorities of all interrupts to the lowest.
        AREG(PLIC_PRIORITY)[i] = 1;
        // Clears the PLIC handlers.
        plic_handlers[i-1] = NULL;
    }

    // Disable all interrupts
    AREG(PLIC_M_ENABLE)[0] = 0;
    AREG(PLIC_M_ENABLE)[1] = 0;

    mi_external_handler = &handle_plic_interrupt;
    const uint32_t mie_setval = BIT(MI_EXTERNAL);
    __asm__ volatile("csrs mie, %0" ::"r"(mie_setval));
}

int plic_handler_register(int source_id, plic_handler_f *handler) {
    if (source_id < 1 || source_id > PLIC_MAX_INTERRUPT) {
        printf("%s: invalid source_id %d\n", __FUNCTION__, source_id);
        return -1;
    }
    if (plic_handlers[source_id - 1] != NULL) {
        printf("%s: double registration %d\n", __FUNCTION__, source_id);
        return -1;
    }
    plic_handlers[source_id - 1] = handler;
    AREG(PLIC_M_ENABLE)[source_id >> 5] |= BIT(source_id & 0x1f);
    return 0;
}
int plic_handler_unregister(int source_id, plic_handler_f *handler) {
    if (source_id < 1 || source_id > PLIC_MAX_INTERRUPT) {
        printf("%s: invalid source_id %d\n", __FUNCTION__, source_id);
        return -1;
    }
    if (plic_handlers[source_id - 1] == NULL) {
        printf("%s: double unregistration %d\n", __FUNCTION__, source_id);
        return -1;
    }
    AREG(PLIC_M_ENABLE)[source_id >> 5] &= ~BIT(source_id & 0x1f);
    plic_handlers[source_id - 1] = NULL;
    return 0;
}

#define MTVEC_MODE_DIRECT 0
#define MTVEC_MODE_VECTORED 1
union mtvec_t {
    struct {
        uint32_t mode : 2; // 0-direct; 1-vectored
        uint32_t base_hi : 30;
    };
    uint32_t raw;
};
_Static_assert(sizeof(union mtvec_t) == 4, "");

void _init_interrupts(void) {
    // Setup handler address
    union mtvec_t mtvec_val = {
    // We have only 3 vectors, just use direct mode.
        .mode = MTVEC_MODE_DIRECT,
        .base_hi = (uint32_t)interrupt_handler >> 2,
    };
    __asm__ volatile("csrw mtvec, %0" ::"r"(mtvec_val.raw));

    // Disable all machine interrupts,
    __asm__ volatile("csrw mie, x0");
    mi_software_handler = &do_nothing_handler;
    mi_timer_handler = &do_nothing_handler;
    mi_external_handler = &do_nothing_handler;

    // but enable the global MIE.
    set_mstatus_mie();

    // Enable machine interrupts individually.
    init_msi(); // Machine software interrupt
    init_mti(); // Machine timer interrupt
    init_mei(); // Machine external interrup, i.e. plic
}
