#ifndef __GPIO_H__
#define __GPIO_H__

// GPIO PIN
// 00     8
// 01    ~9
// 02   ~10
// 03   ~11
// 04   ~12
// 05   ~13/LED
//
// 09    15
// 10    16
// 11   ~17
// 12   ~18/SDA
// 13   ~19/SCL
//
// 16       RXI
// 17       TXO
// 18    2
// 19   ~3
// 20    4
// 21   ~5
// 22   ~6
// 23    7

#include <stdint.h>

enum gpio_iof_type {
    GPIO_IOF_NONE = 0,
    GPIO_IOF_0,
    GPIO_IOF_1,
};

enum gpio_intr_type {
    GPIO_INTR_NONE = 0,
    GPIO_INTR_RISE = 1,
    GPIO_INTR_FALL = 2,
    GPIO_INTR_HIGH = 4,
    GPIO_INTR_LOW  = 8,
};

typedef void (gpio_intr_handler_f)(int gpio, enum gpio_intr_type);
struct gpio_config {
    enum gpio_iof_type iof_sel;  // GPIO_IOF_*
    uint8_t out_xor; // Inverts output.

    // Only used if GPIO_IOF_NONE
    uint8_t input_en;
    uint8_t output_en;
    uint8_t internal_pullup;
    uint8_t drive_strength;  // Not well documented

    uint8_t interrupt_mode; // bitwise-or of GPIO_INTR_*, ignored if intr_callback is NULL.
    gpio_intr_handler_f* intr_handler;
};

void gpio_setup(int gpio, const struct gpio_config* config);
int gpio_read(int gpio);
void gpio_write(int gpio, int val);

#endif  // __GPIO_H__
