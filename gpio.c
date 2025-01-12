#include "gpio.h"

#include "prelude.h"
#include "interrupts.h"
#include "registers.h"

static uint8_t intr_enabled_types[32];
static gpio_intr_handler_f* intr_handlers[32];

static void on_gpio_intr(int source_id) {
    int gpio = source_id - 8;
    if (!((gpio >= 0 && gpio <= 5) ||
          (gpio >= 9 && gpio <= 13)||
          (gpio >= 16&& gpio <= 23))) {
        printf("Invalid GPIO %d\n", gpio);
        return;
    }
    uint8_t im = intr_enabled_types[gpio];
    if (intr_handlers[gpio] == NULL || im == GPIO_INTR_NONE) {
        printf("GPIO %d interrupt has no handler or not enabled\n", gpio);
        return;
    }

    int triggered = 0;
    if ((im & GPIO_INTR_RISE) && (REG(GPIO_RISE_IP) & BIT(gpio))) {
        intr_handlers[gpio](gpio, GPIO_INTR_RISE);
        ++triggered;
        REG(GPIO_RISE_IP) = BIT(gpio);
    }
    if ((im & GPIO_INTR_FALL) && (REG(GPIO_FALL_IP) & BIT(gpio))) {
        intr_handlers[gpio](gpio, GPIO_INTR_FALL);
        ++triggered;
        REG(GPIO_FALL_IP) = BIT(gpio);
    }
    if ((im & GPIO_INTR_HIGH) && (REG(GPIO_HIGH_IP) & BIT(gpio))) {
        intr_handlers[gpio](gpio, GPIO_INTR_HIGH);
        ++triggered;
        REG(GPIO_HIGH_IP) = BIT(gpio);
    }
    if ((im & GPIO_INTR_LOW) && (REG(GPIO_LOW_IP) & BIT(gpio))) {
        intr_handlers[gpio](gpio, GPIO_INTR_LOW);
        ++triggered;
        REG(GPIO_LOW_IP) = BIT(gpio);
    }
    if (!triggered) {
        printf("Phantom GPIO interrupt %d\n", gpio);
    }
}

void gpio_setup(int gpio, const struct gpio_config* config) {
    if (gpio < 0 || gpio >= 32) {
        halt("GPIO index out of range");
    }
    if (config->iof_sel == GPIO_IOF_0) {
        UNSET(REG(GPIO_IOF_SEL), BIT(gpio));
        SET(REG(GPIO_IOF_EN), BIT(gpio));
    } else if (config->iof_sel == GPIO_IOF_1) {
        SET(REG(GPIO_IOF_SEL), BIT(gpio));
        SET(REG(GPIO_IOF_EN), BIT(gpio));
    } else {
        UNSET(REG(GPIO_IOF_EN), BIT(gpio));
        WRITE_BIT(REG(GPIO_INPUT_EN), gpio, config->input_en);
        WRITE_BIT(REG(GPIO_OUTPUT_EN), gpio, config->output_en);
        WRITE_BIT(REG(GPIO_PUE), gpio, config->internal_pullup);
        WRITE_BIT(REG(GPIO_DS), gpio, config->drive_strength);
    }

    uint8_t im = config->interrupt_mode;
    if (config->intr_handler == NULL || config->iof_sel != GPIO_IOF_NONE)
        im = GPIO_INTR_NONE;
    if (im != GPIO_INTR_NONE && !config->input_en) {
        halt("bug: gpio interrupt without input_en");
    }

    // Disable all interrupts.
    // Note the pending bits will still be set
    // TODO: move to above the GPIO_INPUT_EN reg writes.
    UNSET(REG(GPIO_RISE_IE), BIT(gpio));
    UNSET(REG(GPIO_FALL_IE), BIT(gpio));
    UNSET(REG(GPIO_HIGH_IE), BIT(gpio));
    UNSET(REG(GPIO_LOW_IE) , BIT(gpio));

    // Clear all interrupt pending bits.
    // TODO: Clear pending bit in PLIC
    REG(GPIO_RISE_IP) = BIT(gpio);
    REG(GPIO_FALL_IP) = BIT(gpio);
    REG(GPIO_HIGH_IP) = BIT(gpio);
    REG(GPIO_LOW_IP) = BIT(gpio);

    // Set interrupt handler & PLIC
    if (intr_handlers[gpio] == NULL && im != GPIO_INTR_NONE) {
        // Register with PLIC
        plic_handler_register(PLIC_SOURCE_GPIO(gpio), &on_gpio_intr);
        intr_handlers[gpio] = config->intr_handler;
    } else if (intr_handlers[gpio] != NULL && im == GPIO_INTR_NONE) {
        // Unregister
        plic_handler_unregister(PLIC_SOURCE_GPIO(gpio), &on_gpio_intr);
        intr_handlers[gpio] = NULL;
    } else if (intr_handlers[gpio] != NULL && im != GPIO_INTR_NONE) {
        // change handler
        intr_handlers[gpio] = config->intr_handler;
    }
    intr_enabled_types[gpio] = im;

    // Re-enable interrupts
    WRITE_BIT(REG(GPIO_RISE_IE), gpio, im & GPIO_INTR_RISE);
    WRITE_BIT(REG(GPIO_FALL_IE), gpio, im & GPIO_INTR_FALL);
    WRITE_BIT(REG(GPIO_HIGH_IE), gpio, im & GPIO_INTR_HIGH);
    WRITE_BIT(REG(GPIO_LOW_IE) , gpio, im & GPIO_INTR_LOW);
}

int gpio_read(int gpio) {
    return (REG(GPIO_INPUT_VAL) >> gpio) & 0x1;
}

void gpio_write(int gpio, int val) {
    WRITE_BIT(REG(GPIO_OUTPUT_VAL), gpio, val);
}
