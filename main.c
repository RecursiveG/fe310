#include "interrupts.h"
#include "registers.h"
#include "prelude.h"
#include "gpio.h"

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

static int pwm_on = 0;
void pwm(int percentile) {
    // Target freq = 25KHz, bus freq from PLL is 64MHz
    // Output use PWM instance 1, comparator 1, GPIO 19 IOF 1, board pin "~3"
    if (percentile < 0 || percentile > 100) halt("out of range");
    

    if (!pwm_on) {
        puts("Initializing PWM1 CMP1 on GPIO19 IOF1 board PIN~3");
        REG(GPIO_IOF_EN) |= 1<<19;
        REG(GPIO_IOF_SEL) |= 1<<19;

        // x (1/64M) == 1/25K
        //         x == 64M / 25K
        //         x == 2560;
        REG(PWM1_CMP0) = 2560;
        REG(PWM1_CMP1) = 2560 * (100-percentile) / 100;

        uint32_t cfg = 0;
        // pwm scale is 0 so counter counts at 64MHz
        cfg |= 1 << 9; // set pwmzerocmp, this make the counter reset when reach cmp0.
        cfg |= 1 << 10; // set pwmdeglitch
        cfg |= 1 << 12; // set pwmenalways
        REG(PWM1_CFG) = cfg;

        pwm_on = 1;
    }

    printf("Setting PWM to %d%%\n", percentile);
    // First 100-percent are low, then followed by percentile% high
    REG(PWM1_CMP1) = 2560 * (100-percentile) / 100;
}

void btn(int gpio, enum gpio_intr_type type) {
    printf("Interrupt gpio %d type %d\n", gpio, type);
    simulate_input("led");
}

int main(void) {
    printf("Hello RISC-V!\n");
    pwm(30);
    
    struct gpio_config gpiocfg = {
        .iof_sel = GPIO_IOF_NONE,
        .input_en = 1,
        .internal_pullup = 1,
        .interrupt_mode = GPIO_INTR_FALL,
        .intr_handler = &btn,
    };
    gpio_setup(23, &gpiocfg);

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

        } else if (startswith(cmd, "pwm")) {
            char* sval = split_index(cmd, 1);
            if (sval == NULL || sval[0] == '\0') {
                puts("Missing value. Usage: pwm <0~100>");
                if (sval) free(sval);
                continue;
            }
            int val = atoi(sval);
            free(sval);
            if (val < 0 || val > 100) {
                puts("Value out of range. Usage: pwm <0~100>");
            } else {
                pwm(val);
            }

        } else {
            printf("Unknown command: %s\nMaybe check the source code...\n", cmd);
        }
    }
}
