#include "registers.h"
#include "prelude.h"

void stackoverflow(int level) {
    check_heap_smash();
    printf("overflow level=%d\n", level);
    stackoverflow(level + 1);
}

int main(void) {
    printf("Hello RISC-V!\n");

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
