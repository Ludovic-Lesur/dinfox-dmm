/*
 * led.h
 *
 *  Created on: 22 aug 2020
 *      Author: Ludo
 */

#ifndef LED_H
#define LED_H

#include "tim.h"
#include "types.h"

/*** LED structures ***/

typedef enum {
	LED_SUCCESS,
	LED_ERROR_NULL_DURATION,
	LED_ERROR_COLOR,
	LED_ERROR_BASE_LAST = 0x0100
} LED_status_t;

typedef enum {
	LED_COLOR_OFF = TIM3_CHANNEL_MASK_OFF,
	LED_COLOR_RED = TIM3_CHANNEL_MASK_RED,
	LED_COLOR_GREEN = TIM3_CHANNEL_MASK_GREEN,
	LED_COLOR_YELLOW = TIM3_CHANNEL_MASK_YELLOW,
	LED_COLOR_BLUE = TIM3_CHANNEL_MASK_BLUE,
	LED_COLOR_MAGENTA = TIM3_CHANNEL_MASK_MAGENTA,
	LED_COLOR_CYAN = TIM3_CHANNEL_MASK_CYAN,
	LED_COLOR_WHITE = TIM3_CHANNEL_MASK_WHITE,
	LED_COLOR_LAST
} LED_color_t;

/*** LED functions ***/

void LED_init(void);
LED_status_t LED_set_color(LED_color_t color);
LED_status_t LED_start_single_blink(LED_color_t color, uint32_t blink_duration_ms);
uint8_t LED_is_single_blink_done(void);
void LED_stop_blink(void);

#define LED_check_status(error_base) { if (led_status != LED_SUCCESS) { status = error_base + led_status; goto errors; }}
#define LED_stack_error() { ERROR_stack_error(led_status, LED_SUCCESS, ERROR_BASE_LED); }
#define LED_print_error() { ERROR_print_error(led_status, LED_SUCCESS, ERROR_BASE_LED); }

#endif /* LED_H */
