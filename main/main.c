#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "common.h"
#include "led_controller.h"
#include "button_handler.h"
#include "uart_protocol.h"
#include "pwm_manager.h"

static const char *TAG = "MAIN";

// ============= BUTTON CALLBACK HANDLERS =============
static void button_up_callback(button_event_t event) {
    switch (event) {
        case BUTTON_PRESS_SHORT:
            ESP_LOGI(TAG, "Button UP - Single press detected");
            led_increase_brightness();
            break;
        case BUTTON_PRESS_LONG:
            ESP_LOGI(TAG, "Button UP - Long press (continuous increase)");
            led_increase_brightness();
            break;
        case BUTTON_RELEASE:
            ESP_LOGI(TAG, "Button UP - Released");
            break;
    }
}

static void button_down_callback(button_event_t event) {
    switch (event) {
        case BUTTON_PRESS_SHORT:
            ESP_LOGI(TAG, "Button DOWN - Single press detected");
            led_decrease_brightness();
            break;
        case BUTTON_PRESS_LONG:
            ESP_LOGI(TAG, "Button DOWN - Long press (continuous decrease)");
            led_decrease_brightness();
            break;
        case BUTTON_RELEASE:
            ESP_LOGI(TAG, "Button DOWN - Released");
            break;
    }
}

// ============= TIMER CALLBACK =============
// mô phỏng hàm rtos
static void periodic_timer_callback(void *arg) {
    // Xử lý button (10ms interval)
    static uint32_t timer_count = 0;
    timer_count++;

    // quét button
    button_handler_task();

    // PWM  30ms
    if (timer_count % 3 == 0) {
        pwm_fade_task();
    }

    // UART  50ms
    if (timer_count % 5 == 0) {
        uart_process_data();
    }

    // LED status print every 1000ms
    if (timer_count % 100 == 0) {
        led_state_t state = led_get_state();
        ESP_LOGI(TAG, "LED Status - On: %s, Brightness: %d%%, Fading: %s",
                 state.is_on ? "YES" : "NO",
                 state.brightness,
                 state.is_fading ? "YES" : "NO");
    }
}

// ============= MAIN APPLICATION =============
void app_main(void) {
    // khởi tạo modulee
    led_controller_init();
    button_handler_init();
    uart_protocol_init();

    // subr button
    button_up_register_callback(button_up_callback);
    button_down_register_callback(button_down_callback);

   
    // in khởi tjao trnaj tháiban đầu 
    led_print_state();

    const esp_timer_create_args_t timer_args = {
        .callback = periodic_timer_callback,
        .name = "periodic_timer"
    };

    esp_timer_handle_t timer_handle;
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &timer_handle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(timer_handle, 10000));  

    ESP_LOGI(TAG, "Timer started - 10ms interval");

    // Main loop
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
