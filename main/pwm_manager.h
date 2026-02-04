#ifndef __PWM_MANAGER_H__
#define __PWM_MANAGER_H__

#include "common.h"

// ============= PWM MANAGER API =============

void pwm_manager_init(void);

void pwm_set_brightness_immediate(uint8_t brightness);

void pwm_fade_to_brightness(uint8_t target_brightness);

void pwm_stop_fade(void);

bool pwm_is_fading(void);

uint8_t pwm_get_current_brightness(void);

void pwm_fade_task(void);

#endif 
