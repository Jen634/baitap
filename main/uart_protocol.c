
/*
┌──────────────────────────┬─────────────────────┐
│ Function                 │ Hercules Send       │
├──────────────────────────┼─────────────────────┤
│ LED ON (fade to 100%)    │ 55 AA 01 01 FF      │
│ LED OFF (fade to 0%)     │ 55 AA 01 00 FE      │
│ Brightness 0%            │ 55 AA 03 00 FC      │
│ Brightness 25%           │ 55 AA 03 19 E5      │
│ Brightness 50%           │ 55 AA 03 32 CE      │
│ Brightness 75%           │ 55 AA 03 4B B7      │
│ Brightness 100%          │ 55 AA 03 64 98      │
|──────────────────────────┴─────────────────────┘

│ gửi qua hercles tích chọn mã hex và không cách  ex: 55AA0101FF
*/
#include "uart_protocol.h"
#include "led_controller.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "string.h"
#include "stdio.h"
#include "stdarg.h"

static const char *TAG = "UART";

// ============= UART BUFFER =============
static uint8_t uart_buffer[UART_BUF_SIZE];
static uint16_t uart_buffer_pos = 0;

// ============= checksm ==================
static uint8_t calculate_checksum(uint8_t header1, uint8_t header2, uint8_t cmd, uint8_t value) {
    return header1 ^ header2 ^ cmd ^ value;
}

static uart_error_t validate_frame(uart_frame_t *frame) {
    // check header có chuẩn không 
    if (frame->header1 != UART_HEADER_BYTE1) {
        return UART_ERR_HEADER1;
    }

    if (frame->header2 != UART_HEADER_BYTE2) {
        return UART_ERR_HEADER2;
    }
    // kiểm tra lệnh có đúng không
    if (frame->cmd != LED_CMD_PWM_MODE && frame->cmd != LED_CMD_ON_OFF) {
        return UART_ERR_INVALID_CMD;
    }
   
    if (frame->cmd == LED_CMD_PWM_MODE) {
        if (frame->value > 100) {
            return UART_ERR_INVALID_VALUE;
        }

    } else if (frame->cmd == LED_CMD_ON_OFF) {
        if (frame->value != LED_STATE_OFF && frame->value != LED_STATE_ON) {
            return UART_ERR_INVALID_VALUE;
        }
    }

    // Check checksum
    uint8_t expected_checksum = calculate_checksum(frame->header1, frame->header2, frame->cmd, frame->value);
    if (frame->checksum != expected_checksum) {
        return UART_ERR_CHECKSUM;
    }

    return UART_ERR_OK;
}

static void process_valid_frame(uart_frame_t *frame) {
    ESP_LOGI(TAG, "Processing valid frame - CMD: 0x%02X, Value: 0x%02X",
             frame->cmd, frame->value);

    if (frame->cmd == LED_CMD_PWM_MODE) {
        uart_log("[OK] PWM Mode - Brightness: %d%%\r\n", frame->value);
        led_set_brightness_uart(frame->value);
        uart_send_ack(frame->cmd, frame->value);
    } else if (frame->cmd == LED_CMD_ON_OFF) {
        bool turn_on = (frame->value == LED_STATE_ON);
        uart_log("[OK] %s LED\r\n", turn_on ? "ON" : "OFF");
        led_set_on_off_uart(turn_on);
        uart_send_ack(frame->cmd, frame->value);
    }
}

static const char *get_error_message(uart_error_t error) {
    switch (error) {
        case UART_ERR_HEADER1:
            return "Invalid Header Byte 1 (expected 0x55)";
        case UART_ERR_HEADER2:
            return "Invalid Header Byte 2 (expected 0xAA)";
        case UART_ERR_INVALID_CMD:
            return "Invalid Command (0x03=PWM, 0x01=ON/OFF)";
        case UART_ERR_INVALID_VALUE:
            return "Invalid Value (PWM: 0-100, ON/OFF: 0x00/0x01)";
        case UART_ERR_CHECKSUM:
            return "Checksum Mismatch";
        default:
            return "Unknown Error";
    }
}


void uart_protocol_init(void) {
    ESP_LOGI(TAG, "Initializing UART Protocol...");

    // UART configuration
    uart_config_t uart_config = {
        .baud_rate = UART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_DEFAULT
    };

    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, UART_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN,
                                  UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    uart_buffer_pos = 0;
    memset(uart_buffer, 0, UART_BUF_SIZE);

    ESP_LOGI(TAG, "UART initialized - Baudrate: %d, TX: GPIO%d, RX: GPIO%d",
             UART_BAUDRATE, UART_TX_PIN, UART_RX_PIN);
}

void uart_process_data(void) {
    uint8_t received_byte;
    int read_len = 0;

    // Read dữ liệu từ UART
    while ((read_len = uart_read_bytes(UART_PORT, &received_byte, 1, 0)) > 0) {
        // Log mỗi byte nhận được
        uart_log("[RX] 0x%02X ", received_byte);
        
        // Nếu buffer đầy, reset
        if (uart_buffer_pos >= UART_BUF_SIZE - 1) {
            uart_buffer_pos = 0;
        }

        uart_buffer[uart_buffer_pos++] = received_byte;

        // Kiểm tra nếu có frame hoàn chỉnh
        if (uart_buffer_pos >= UART_FRAME_MIN_LEN) {
            // Tìm header
            int header_index = -1;
            for (int i = 0; i < uart_buffer_pos - 1; i++) {
                if (uart_buffer[i] == UART_HEADER_BYTE1 && uart_buffer[i + 1] == UART_HEADER_BYTE2) {
                    header_index = i;
                    break;
                }
            }

            if (header_index != -1) {
                // Kiểm tra nếu có frame hoàn chỉnh
                if (uart_buffer_pos >= header_index + UART_FRAME_MIN_LEN) {
                    uart_frame_t frame = {0};
                    frame.header1 = uart_buffer[header_index];
                    frame.header2 = uart_buffer[header_index + 1];
                    frame.cmd = uart_buffer[header_index + 2];
                    frame.value = uart_buffer[header_index + 3];
                    frame.checksum = uart_buffer[header_index + 4];

                    // Log frame nhận được
                    uart_log("\r\n[FRAME] H1=0x%02X H2=0x%02X CMD=0x%02X VAL=0x%02X CRC=0x%02X\r\n",
                            frame.header1, frame.header2, frame.cmd, frame.value, frame.checksum);

                    // Calculate expected checksum
                    uint8_t expected_crc = calculate_checksum(frame.header1, frame.header2, frame.cmd, frame.value);
                    uart_log("[CHECK] Expected CRC=0x%02X, Got CRC=0x%02X ", expected_crc, frame.checksum);

                    // Validate frame
                    uart_error_t error = validate_frame(&frame);

                    if (error == UART_ERR_OK) {
                        uart_log("→ OK\r\n");
                        ESP_LOGI(TAG, "Valid frame received");
                        process_valid_frame(&frame);
                    } else {
                        uart_log("→ FAIL\r\n");
                        ESP_LOGI(TAG, "Invalid frame - Error: %d", error);
                        uart_send_error(error, get_error_message(error));
                    }

                    // Remove processed frame from buffer
                    uart_buffer_pos -= (header_index + UART_FRAME_MIN_LEN);
                    if (uart_buffer_pos > 0) {
                        memmove(uart_buffer, uart_buffer + header_index + UART_FRAME_MIN_LEN, uart_buffer_pos);
                    }
                } else {
                    uart_log("(waiting for more)...");
                    break;  // Chưa đủ dữ liệu cho frame hoàn chỉnh
                }
            } else {
                // Không tìm thấy header, reset buffer
                if (uart_buffer_pos > 2) {
                    uart_log("\r\n[WARN] No header found, clearing buffer\r\n");
                    uart_buffer_pos = 0;
                }
            }
        }
    }
}

void uart_log(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
    fflush(stdout);
}

void uart_send_ack(uint8_t cmd, uint8_t value) {
    char ack_msg[100];
    snprintf(ack_msg, sizeof(ack_msg),
             "[ACK] CMD=0x%02X, Value=0x%02X, Checksum=0x%02X\r\n",
             cmd, value, calculate_checksum(UART_HEADER_BYTE1, UART_HEADER_BYTE2, cmd, value));
    uart_log("%s", ack_msg);
    ESP_LOGI(TAG, "ACK sent: CMD=0x%02X", cmd);
}

void uart_send_error(uart_error_t error_code, const char *detail) {
    uart_log("[ERROR] Code=%d, Message: %s\r\n", error_code, detail);
    ESP_LOGE(TAG, "Error sent: Code=%d", error_code);
}