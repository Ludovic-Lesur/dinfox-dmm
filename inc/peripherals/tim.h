/*
 * tim.h
 *
 *  Created on: 22 aug. 2020
 *      Author: Ludo
 */

#ifndef __TIM_H__
#define __TIM_H__

#include "types.h"

/*** TIM structures ***/

typedef enum {
	TIM_SUCCESS = 0,
	TIM_ERROR_NULL_PARAMETER,
	TIM_ERROR_INTERRUPT_TIMEOUT,
	TIM_ERROR_BASE_LAST = 0x0100
} TIM_status_t;

// Color bit masks defined as 0b<CH4><CH3><CH2><CH1>
typedef enum {
	TIM3_CHANNEL_MASK_OFF = 0b0000,
	TIM3_CHANNEL_MASK_RED = 0b0010,
	TIM3_CHANNEL_MASK_GREEN = 0b0100,
	TIM3_CHANNEL_MASK_YELLOW = 0b0110,
	TIM3_CHANNEL_MASK_BLUE = 0b1000,
	TIM3_CHANNEL_MASK_MAGENTA = 0b1010,
	TIM3_CHANNEL_MASK_CYAN = 0b1100,
	TIM3_CHANNEL_MASK_WHITE	= 0b1110
} TIM3_channel_mask_t;

/*** TIM functions ***/

void TIM3_init(void);
void TIM3_start(TIM3_channel_mask_t led_color);
void TIM3_stop(void);

void TIM22_init(void);
void TIM22_start(uint32_t led_blink_period_ms);
void TIM22_stop(void);
uint8_t TIM22_is_single_blink_done(void);

void TIM21_init(void);
TIM_status_t TIM21_get_lsi_frequency(uint32_t* lsi_frequency_hz);
void TIM21_disable(void);

#define TIM21_check_status(error_base) { if (tim21_status != TIM_SUCCESS) { status = error_base + tim21_status; goto errors; }}
#define TIM21_stack_error() { ERROR_check_status(tim21_status, TIM_SUCCESS, ERROR_BASE_TIM21); }
#define TIM21_stack_error_print() { ERROR_check_status_print(tim21_status, TIM_SUCCESS, ERROR_BASE_TIM21); }

#endif /* __TIM_H__ */
