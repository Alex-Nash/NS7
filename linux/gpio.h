#ifndef GPIO_H
#define GPIO_H

#include <inttypes.h>

#define GPIO_MB_RESET_PIN    960

int mb_start();

int mb_stop();

int set_gpio_to_hi(uint16_t gpio_pin);

int set_gpio_to_low(uint16_t gpio_pin);

int init_gpio (uint16_t gpio_pin);

int close_gpio (uint16_t gpio_pin);


#endif
