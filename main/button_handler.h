#ifndef __BUTTON_HANDLER_H__
#define __BUTTON_HANDLER_H__

#include "common.h"

// ============= BUTTON EVENTS =============
typedef enum {
    BUTTON_PRESS_SHORT = 0,                 // Single press
    BUTTON_PRESS_LONG = 1,                  // Long press (held)
    BUTTON_RELEASE = 2
} button_event_t;

typedef void (*button_callback_t)(button_event_t event);

// ============= BUTTON HANDLER API =============

void button_handler_init(void);


void button_up_register_callback(button_callback_t callback);

void button_down_register_callback(button_callback_t callback);


void button_handler_task(void);

#endif // __BUTTON_HANDLER_H__
