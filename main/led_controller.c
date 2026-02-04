#include "led_controller.h"
#include "pwm_manager.h"
#include "esp_log.h"
#include "stdio.h"

static const char *TAG = "LED_CTRL";

// ============= GLOBAL LED STATE =============
led_state_t g_led_state = {
    .brightness = 0,
    .is_on = false,
    .mode = LED_MODE_PWM,
    .is_fading = false
};

// ============= LED CONTROLLER IMPLEMENTATION =============
void led_controller_init(void) {
    ESP_LOGI(TAG, "Initializing LED Controller...");

    pwm_manager_init();

    g_led_state.brightness = 0;
    g_led_state.is_on = false;
    g_led_state.mode = LED_MODE_PWM;
    g_led_state.is_fading = false;

    // Set LED to OFF initially
    pwm_set_brightness_immediate(0);

    ESP_LOGI(TAG, "LED Controller initialized");
}

void led_increase_brightness(void) {
    // Nếu LED tắt, bật nó trước
    if (!g_led_state.is_on) {
        g_led_state.is_on = true;
        g_led_state.brightness = 0;
    }

    if (g_led_state.brightness < 100) {
        g_led_state.brightness += 1;
        pwm_fade_to_brightness(g_led_state.brightness);
        g_led_state.is_fading = pwm_is_fading();

        ESP_LOGI(TAG, "Increase brightness: %d%%", g_led_state.brightness);
    }
}

void led_decrease_brightness(void) {
    if (g_led_state.brightness > 0) {
        g_led_state.brightness -= 1;
        pwm_fade_to_brightness(g_led_state.brightness);
        g_led_state.is_fading = pwm_is_fading();

        ESP_LOGI(TAG, "Decrease brightness: %d%%", g_led_state.brightness);

        if (g_led_state.brightness == 0) {
            g_led_state.is_on = false;
        }
    }
}

void led_set_brightness_uart(uint8_t brightness) {
    if (brightness > 100) brightness = 100;

    g_led_state.brightness = brightness;
    g_led_state.is_on = (brightness > 0);

    pwm_fade_to_brightness(brightness);
    g_led_state.is_fading = pwm_is_fading();

    ESP_LOGI(TAG, "Set brightness via UART: %d%%", brightness);
}

void led_set_on_off_uart(bool on) {
    g_led_state.is_on = on;

    if (on) {
       
        if (g_led_state.brightness == 0) {
            g_led_state.brightness = 100;
        }
    } else {
       
        g_led_state.brightness = 0;
    }

    pwm_fade_to_brightness(g_led_state.brightness);
    g_led_state.is_fading = pwm_is_fading();

    ESP_LOGI(TAG, "Set LED %s via UART", on ? "ON" : "OFF");
}

led_state_t led_get_state(void) {
    g_led_state.brightness = pwm_get_current_brightness();
    g_led_state.is_fading = pwm_is_fading();
    return g_led_state;
}

void led_print_state(void) {
    led_state_t state = led_get_state();
    printf("\n========== LED STATUS ==========\n");
    printf("State: %s\n", state.is_on ? "ON" : "OFF");
    printf("Brightness: %d%%\n", state.brightness);
    printf("Mode: %s\n", state.mode == LED_MODE_PWM ? "PWM" : "ON/OFF");
    printf("Fading: %s\n", state.is_fading ? "YES" : "NO");
    printf("================================\n\n");
}
