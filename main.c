#include "interrupts.h"
#include "registers.h"
#include "prelude.h"

void stackoverflow(int level) {
    check_heap_smash();
    printf("overflow level=%d\n", level);
    stackoverflow(level + 1);
}

static int led_on = -1;
void toggle_led() {
    if (led_on < 0) {
        puts("Initializing LED...");
        REG(GPIO_OUTPUT_VAL) &= 0xFFFFFFDFU;
        REG(GPIO_OUTPUT_EN) |= 0x20;
        led_on = 0;
    }

    if (led_on == 0) {
        REG(GPIO_OUTPUT_VAL) |= 0x20U;
        puts("LED turned " COLOR_BLUE "on" COLOR_RESET);
        led_on = 1;
    } else {
        REG(GPIO_OUTPUT_VAL) &= 0xFFFFFFDFU;
        puts("LED turned " COLOR_WHITE "off" COLOR_RESET);
        led_on = 0;
    }
}

int main(void) {
    printf("Hello RISC-V!\n");
    while(1) {
        char cmd[256];
        printf("cmd>");
        gets(cmd);
        if (cmd[0] == '\0') continue;

        if (0 == strcmp(cmd, "sqrt")) {
            puts("Finding sqrt(2) using Newton's method...");
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

        } else if (0 == strcmp(cmd, "sw_int")) {
            puts("Now triggering a software interrupt to the core.");
            REG(CLINT_MSIP) = 1;

        } else if (0 == strcmp(cmd, "stackoverflow")) {
            stackoverflow(0);

        } else if (0 == strcmp(cmd, "halt")) {
            halt("user requested halt");

        } else if (0 == strcmp(cmd, "greet")) {
            printf("Your name? ");
            char str[256];
            gets(str);
            if (strlen(str) == 0) {
                memcpy(str, "world", 6);
            }
            printf("Hello %s!\n", str);

        } else if (0 == strcmp(cmd, "sleep")) {
            puts("Sleeping 3 seconds...");
            sleep(3);

        } else if (0 == strcmp(cmd, "led")) {
            toggle_led();
        
        } else if (0 == strcmp(cmd, "color")) {
            puts(COLOR_RED "red " COLOR_GREEN "green " COLOR_BLUE "blue " COLOR_WHITE "white" COLOR_RESET);

        } else {
            printf("Unknown command: %s\nMaybe check the source code...\n", cmd);
        }
    }
}
