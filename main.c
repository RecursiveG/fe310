#include "interrupts.h"
#include "registers.h"
#include "prelude.h"

void stackoverflow(int level) {
    check_heap_smash();
    printf("overflow level=%d\n", level);
    stackoverflow(level + 1);
}

static void rx_interrupt(void) {
    int data = REG(UART0_RXDATA);
    if (data < 0) halt("rx interrupt has no data");

    // Backspace, doesn't work in middle of a line.
    if (data == 127) {
        printf("\b \b");
        return;
    }

    // User pressing ENTER generates a CR.
    if (data == '\r') data = '\n';

    // Echo it back.
    putchar(data);
}

int main(void) {
    printf("Hello RISC-V!\n");
    REG(CLINT_MSIP) = 1;

    // Init UART RX
    REG(GPIO_IOF_EN)  |= 1<<16;
    REG(UART0_RXCTRL) = 1;
    REG(UART0_IE) = 2;
    plic_handler_register(PLIC_SOURCE_UART0, &rx_interrupt);

    puts("UART RX ready. Try typing something. Continue in 10 seconds...");
    sleep(10);
    plic_handler_unregister(PLIC_SOURCE_UART0, &rx_interrupt);
    puts("UART RX disabled.");

    puts("Finding sqrt(2) using Newton's method.");
    double n = 2;
    double x = n/2;
    int iter = 0;
    while (1) {
        double next = (x + n / x) / 2;
        double err = next - x;
        printf("Iteration #%d value=%f error=%f\n", ++iter, next, err);
        if (err < 0) err = -err;
        if (err < 1e-6) break;
        x = next;
    }

    int led_on = 0;
    int counter = -10;
    REG(GPIO_OUTPUT_EN) |= 0x20;
    while (1) {
        printf("[%s] counter=%d\n", "INFO", counter++);

        if (counter == 5) stackoverflow(0);

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
