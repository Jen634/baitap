#include "button_handler.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "BTN_HDL";

// ============= BUTTON STATE =============
typedef struct {
    uint16_t press_count;
    uint16_t release_count;
    bool is_pressed;
    uint32_t press_time_ms;
    bool long_press_triggered;
    uint32_t repeat_time_ms;           
} button_state_t;

typedef struct {
    button_state_t state_up;
    button_state_t state_down;
    button_callback_t callback_up;
    button_callback_t callback_down;
} button_handler_t;

static button_handler_t g_button_handler = {0};
static uint32_t last_scan_time = 0;

// ============= STATIC FUNCTIONS =============
static bool read_button_level(gpio_num_t pin) {
    return gpio_get_level(pin) == 0;  // Nút nhấn LOW when pressed
}

static void process_button(gpio_num_t pin, button_state_t *state, button_callback_t callback) {
    bool current_level = read_button_level(pin);

    if (current_level && !state->is_pressed) {
        // Button pressed (debounce)
        state->press_count++;
        if (state->press_count >= BUTTON_DEBOUNCE_MS / 10) {
            state->is_pressed = true;
            state->press_time_ms = 0;
            state->long_press_triggered = false;
            state->repeat_time_ms = 0;           
            state->press_count = 0;
            state->release_count = 0;

            if (callback) {
                callback(BUTTON_PRESS_SHORT);
            }
            ESP_LOGI(TAG, "Button GPIO %d pressed", pin);
        }
    } else if (current_level && state->is_pressed) {
        // Button held
        state->press_time_ms += 10;
        state->repeat_time_ms += 10;              

        if (state->press_time_ms >= BUTTON_LONG_PRESS_MS && !state->long_press_triggered) {
            state->long_press_triggered = true;
            state->repeat_time_ms = 0;             
            if (callback) {
                callback(BUTTON_PRESS_LONG);
            }
            ESP_LOGI(TAG, "Button GPIO %d long press triggered", pin);
        }

        // Handle repeat events
        if (state->long_press_triggered && state->repeat_time_ms >= BUTTON_REPEAT_INTERVAL_MS) {
            state->repeat_time_ms = 0; 
            if (callback) {
                callback(BUTTON_PRESS_SHORT);  
            }
            
        }
        // ================================================
        
    } else if (!current_level && state->is_pressed) {
        // Button released
        state->release_count++;
        if (state->release_count >= BUTTON_DEBOUNCE_MS / 10) {
            state->is_pressed = false;
            state->press_count = 0;
            state->release_count = 0;
            state->repeat_time_ms = 0;             // ← MỚI: Reset repeat timer

            if (callback) {
                callback(BUTTON_RELEASE);
            }
            ESP_LOGI(TAG, "Button GPIO %d released", pin);
        }
    } else {
        // Button not pressed
        state->press_count = 0;
        state->release_count = 0;
        state->repeat_time_ms = 0;                 // ← MỚI: Reset repeat timer
    }
}

// ============= BUTTON HANDLER IMPLEMENTATION =============
void button_handler_init(void) {
    ESP_LOGI(TAG, "Initializing button handler...");

    // Configure button pins as input
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_UP_PIN) | (1ULL << BUTTON_DOWN_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // Initialize button states
    g_button_handler.callback_up = NULL;
    g_button_handler.callback_down = NULL;
    memset(&g_button_handler.state_up, 0, sizeof(button_state_t));
    memset(&g_button_handler.state_down, 0, sizeof(button_state_t));

    ESP_LOGI(TAG, "Button handler initialized");
    ESP_LOGI(TAG, "Button UP pin: GPIO%d, Button DOWN pin: GPIO%d",
             BUTTON_UP_PIN, BUTTON_DOWN_PIN);
    ESP_LOGI(TAG, "Long press: %dms, Repeat interval: %dms",
             BUTTON_LONG_PRESS_MS, BUTTON_REPEAT_INTERVAL_MS);
}

void button_up_register_callback(button_callback_t callback) {
    g_button_handler.callback_up = callback;
    ESP_LOGI(TAG, "Button UP callback registered");
}

void button_down_register_callback(button_callback_t callback) {
    g_button_handler.callback_down = callback;
    ESP_LOGI(TAG, "Button DOWN callback registered");
}

void button_handler_task(void) {
    // Scan every 10ms
    process_button(BUTTON_UP_PIN, &g_button_handler.state_up, g_button_handler.callback_up);
    process_button(BUTTON_DOWN_PIN, &g_button_handler.state_down, g_button_handler.callback_down);
}