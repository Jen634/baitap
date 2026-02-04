#ifndef __UART_PROTOCOL_H__
#define __UART_PROTOCOL_H__

#include "common.h"

// ============= UART PROTOCOL API =============

void uart_protocol_init(void);

void uart_log(const char *format, ...);

void uart_process_data(void);

void uart_send_ack(uint8_t cmd, uint8_t value);

void uart_send_error(uart_error_t error_code, const char *detail);

#endif 
