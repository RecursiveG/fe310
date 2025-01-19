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
static int current_percentile = 0;
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
    current_percentile = percentile;
}

// PIN 7:Toggle LED
// PIN~6:Toggle PWM duty cycle
void on_button_press(int gpio, enum gpio_intr_type type) {
    if (gpio == 23 && type == GPIO_INTR_FALL) {
        simulate_input("led");
    } else if (gpio == 22 && type == GPIO_INTR_FALL) {
        simulate_input("pwm next");
    } else {
        printf("Unknown interrupt for GPIO %d type %d\n", gpio, type);
    }
}

#define I2C_CMD_IACK    0x01
#define I2C_CMD_NACK    0x08
#define I2C_CMD_WRITE   0x10
#define I2C_CMD_READ    0x20
#define I2C_CMD_STOP    0x40
#define I2C_CMD_START   0x80

// return the status register
int i2c_wait_completion(void) {
    int status;
    uint64_t deadline = REG64(CLINT_MTIME) + 10000;
    while (1) {
        status = REG(I2C_SR);
        if ((status & 0x2) == 0) {
            break;
        }
        if (REG64(CLINT_MTIME) > deadline) {
            halt("I2C transaction not complete in time");
        }
    }
    return status;
}

int i2c_issue_stop(void) {
    REG(I2C_CR) = I2C_CMD_STOP;
    i2c_wait_completion();
}

static int i2c_initialized = 0;
// MCP9808 read temperature
void i2c_read_temperature(void) {
    if (!i2c_initialized) {
        printf("Initializing I2C on GPIO 12/13 IOF 0 PIN 18/19...\n");
        struct gpio_config gpiocfg = {
            .iof_sel = GPIO_IOF_0,
            .input_en = 1,
        };
        gpio_setup(12, &gpiocfg);
        gpio_setup(13, &gpiocfg);

        // prescale = bus_freq / (5 * i2c_freq) - 1
        // bus_freq=64MHz  i2c_freq=400KHz  prescale=31
        // Note this cannot give the exact frequency.
        REG(I2C_PRER_HI) = 31 >> 8;
        REG(I2C_PRER_LO) = 31 & 0xff;

        REG(I2C_CTR) = 0x80;  // Enable bit

        i2c_initialized = 1;
    }

    int status;
    REG(I2C_TXR) = 0x30;  // Write to device address 0x18
    REG(I2C_CR) = I2C_CMD_START | I2C_CMD_WRITE;
    status = i2c_wait_completion();
    if (status & 0x80) { printf("NACK :-(\n"); i2c_issue_stop(); return; }

    REG(I2C_TXR) = 0x05;  // Set read pointer to 0x5 (temperature reg)
    REG(I2C_CR) = I2C_CMD_WRITE | I2C_CMD_STOP;
    status = i2c_wait_completion();
    if (status & 0x80) { printf("NACK :-(\n"); return; }

    // Signal timing for restart is broken (hw bug?), have to stop then start.

    REG(I2C_TXR) = 0x31;  // Read from device address 0x18
    REG(I2C_CR) = I2C_CMD_START | I2C_CMD_WRITE;
    status = i2c_wait_completion();
    if (status & 0x80) { printf("NACK :-(\n"); i2c_issue_stop(); return; }

    REG(I2C_CR) = I2C_CMD_READ;  // Read high byte
    status = i2c_wait_completion();
    uint32_t hi = REG(I2C_RXR);

    REG(I2C_CR) = I2C_CMD_READ | I2C_CMD_NACK | I2C_CMD_STOP;  // Read low byte
    status = i2c_wait_completion();
    uint32_t lo = REG(I2C_RXR);

    // Convert temperature to decimal.
    double temp = (double)((int16_t)((hi << 11) | (lo << 3)) >> 3) / 16.0;
    printf("Temperature=%f\n", temp);
}

int main(void) {
    printf("Hello RISC-V!\n");
    
    struct gpio_config gpiocfg = {
        .iof_sel = GPIO_IOF_NONE,
        .input_en = 1,
        .internal_pullup = 1,
        .interrupt_mode = GPIO_INTR_FALL,
        .intr_handler = &on_button_press,
    };
    gpio_setup(22, &gpiocfg);
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

        } else if (0 == strcmp(cmd, "i2c")) {
            for (int i = 0; i < 10; ++i) {
                i2c_read_temperature();
                sleep(1);
            }

        } else if (startswith(cmd, "pwm")) {
            char* sval = split_index(cmd, 1);
            if (sval == NULL || sval[0] == '\0') {
                puts("Missing value. Usage: pwm <0~100> or \"pwm next\"");
                if (sval) free(sval);
                continue;
            }
            int val = atoi(sval);
            if (strcmp(sval, "next") == 0) {
                if (current_percentile < 20) val = 20;
                else if (current_percentile < 40) val = 40;
                else if (current_percentile < 60) val = 60;
                else if (current_percentile < 80) val = 80;
                else if (current_percentile < 100) val = 100;
                else val = 20;
                printf("Next PWM value: %d\n", val);
            }
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
