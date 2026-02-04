#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdint.h>
#include <stdbool.h>
#include "driver/gpio.h"
#include "string.h"


// ============= GPIO PIN DEFINITIONS =============
#define LED_PIN             GPIO_NUM_0      
#define BUTTON_UP_PIN       GPIO_NUM_4     
#define BUTTON_DOWN_PIN     GPIO_NUM_5     

// ============= PWM CONFIGURATIONS =============
#define PWM_FREQUENCY       1000            
#define PWM_RESOLUTION      10              // 10-bit 
#define PWM_MAX_DUTY        1023            
#define PWM_STEP            1               //  càng giảm thì thời gian pwm càng lâu hơn 
#define PWM_FADE_DELAY_MS   30              

// ============= BUTTON CONFIGURATIONS =============
#define BUTTON_DEBOUNCE_MS  20              // Debounce  
#define BUTTON_LONG_PRESS_MS 500            
#define BUTTON_REPEAT_INTERVAL_MS 100       

// ============= UART CONFIGURATIONS =============
#define UART_PORT           UART_NUM_0      // UART0 cho giao tiếp
#define UART_BAUDRATE       115200
#define UART_TX_PIN         GPIO_NUM_1     // TX pin
#define UART_RX_PIN         GPIO_NUM_3    // RX pin
#define UART_BUF_SIZE       2048

// ============= UART PROTOCOL DEFINITIONS =============
#define UART_HEADER_BYTE1   0x55
#define UART_HEADER_BYTE2   0xAA

#define LED_CMD_PWM_MODE    0x03            // Chế độ PWM
#define LED_CMD_ON_OFF      0x01           

#define LED_STATE_OFF       0x00
#define LED_STATE_ON        0x01

// ============= UART FRAME STRUCTURE =============
// Header(2) + CMD(1) + Value(1) + Checksum(1)
#define UART_FRAME_MIN_LEN  5               
#define UART_FRAME_MAX_LEN  10

// ============= SYSTEM STATES =============
typedef enum {
    LED_MODE_PWM = 0,
    LED_MODE_ON_OFF = 1
} led_mode_t;

typedef struct {
    uint8_t brightness;                     // xung pwm
    bool is_on;
    led_mode_t mode;
    bool is_fading;                         
} led_state_t;

// form uart    
typedef struct {
    uint8_t header1;
    uint8_t header2;
    uint8_t cmd;
    uint8_t value;
    uint8_t checksum;
} uart_frame_t;

typedef enum {
    UART_ERR_OK = 0,
    UART_ERR_HEADER1 = 1,
    UART_ERR_HEADER2 = 2,
    UART_ERR_INVALID_CMD = 3,
    UART_ERR_INVALID_VALUE = 4,
    UART_ERR_CHECKSUM = 5
} uart_error_t;

// ============= FUNCTION DECLARATIONS =============

extern led_state_t g_led_state;

#endif // __COMMON_H__
