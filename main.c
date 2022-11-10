#include "registers.h"
#include "prelude.h"

int main(void) {
    puts("Hello RISC-V!");

    int led_on = 0;
    int counter = -10;
    REG(GPIO_OUTPUT_EN) |= 0x20;
    while (1) {
        char* str = (char*) malloc(256);
        memcpy(str, "loop ", 5);
        itoa(counter++, str + 5);
        puts(str);
        free(str);

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
