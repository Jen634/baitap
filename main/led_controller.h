#ifndef __LED_CONTROLLER_H__
#define __LED_CONTROLLER_H__

#include "common.h"

// ============= LED CONTROLLER API =============

void led_controller_init(void);

void led_increase_brightness(void);

void led_decrease_brightness(void);

void led_set_brightness_uart(uint8_t brightness);

void led_set_on_off_uart(bool on);

led_state_t led_get_state(void);

void led_print_state(void);

#endif // __LED_CONTROLLER_H__
