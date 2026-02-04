#include "pwm_manager.h"
#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "PWM_MGR";

// ============= PWM STATE =============
typedef struct {
    uint8_t current_brightness;             // 0-100 (%)
    uint8_t target_brightness;
    bool is_fading;
} pwm_state_t;

static pwm_state_t pwm_state = {
    .current_brightness = 0,
    .target_brightness = 0,
    .is_fading = false
};

// ============= UTILITY FUNCTIONS =============
static uint16_t brightness_to_duty(uint8_t brightness_percent) {
    // Convert 0-100 to 0-1023
    // NOTE: For ACTIVE-LOW LED, we INVERT the value
    // brightness 100% = duty 0 (LED ON fully)
    // brightness 0% = duty 1023 (LED OFF)
    uint16_t duty = (uint16_t)((brightness_percent * PWM_MAX_DUTY) / 100);
    return PWM_MAX_DUTY - duty;  // INVERT for active-low
}

static uint8_t duty_to_brightness(uint16_t duty) {
    // Convert 0-1023 back to 0-100
    // INVERT back since it's active-low
    uint16_t inverted_duty = PWM_MAX_DUTY - duty;
    return (uint8_t)((inverted_duty * 100) / PWM_MAX_DUTY);
}

// ============= PWM MANAGER IMPLEMENTATION =============
void pwm_manager_init(void) {
    ESP_LOGI(TAG, "Initializing PWM Manager (ACTIVE-LOW LED)...");

    // Cấu hình LED PWM controller
    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = PWM_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_config));

    // Cấu hình LED PWM channel
    ledc_channel_config_t channel_config = {
        .gpio_num = LED_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = PWM_MAX_DUTY,  // Start at OFF (active-low, 1023 = OFF)
        .hpoint = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_config));

    pwm_state.current_brightness = 0;
    pwm_state.target_brightness = 0;
    pwm_state.is_fading = false;

    ESP_LOGI(TAG, "PWM initialized on GPIO %d, Freq: %d Hz (ACTIVE-LOW)", LED_PIN, PWM_FREQUENCY);
    ESP_LOGI(TAG, "Note: This is for ACTIVE-LOW LED (LED ON when GPIO LOW)");
}

void pwm_set_brightness_immediate(uint8_t brightness) {
    if (brightness > 100) brightness = 100;

    uint16_t duty = brightness_to_duty(brightness);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    pwm_state.current_brightness = brightness;
    pwm_state.target_brightness = brightness;
    pwm_state.is_fading = false;

    ESP_LOGI(TAG, "Set brightness immediate: %d%% (duty: %d, inverted for active-low)", brightness, duty);
}

void pwm_fade_to_brightness(uint8_t target_brightness) {
    if (target_brightness > 100) target_brightness = 100;

    pwm_state.target_brightness = target_brightness;
    pwm_state.is_fading = true;

    ESP_LOGI(TAG, "Start fading to brightness: %d%% (current: %d%%)",
             target_brightness, pwm_state.current_brightness);
}

void pwm_stop_fade(void) {
    pwm_state.is_fading = false;
    ESP_LOGI(TAG, "Fade stopped at brightness: %d%%", pwm_state.current_brightness);
}

bool pwm_is_fading(void) {
    return pwm_state.is_fading;
}

uint8_t pwm_get_current_brightness(void) {
    return pwm_state.current_brightness;
}

void pwm_fade_task(void) {
    if (!pwm_state.is_fading) {
        return;
    }

    // Tính hướng fade
    if (pwm_state.current_brightness < pwm_state.target_brightness) {
        // Fade up
        pwm_state.current_brightness += PWM_STEP;
        if (pwm_state.current_brightness >= pwm_state.target_brightness) {
            pwm_state.current_brightness = pwm_state.target_brightness;
            pwm_state.is_fading = false;
        }
    } else if (pwm_state.current_brightness > pwm_state.target_brightness) {
        // Fade down
        if (pwm_state.current_brightness >= PWM_STEP) {
            pwm_state.current_brightness -= PWM_STEP;
        } else {
            pwm_state.current_brightness = 0;
        }

        if (pwm_state.current_brightness <= pwm_state.target_brightness) {
            pwm_state.current_brightness = pwm_state.target_brightness;
            pwm_state.is_fading = false;
        }
    }

    // Update duty (inverted for active-low)
    uint16_t duty = brightness_to_duty(pwm_state.current_brightness);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}