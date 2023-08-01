/*
 * lptim.h
 *
 *  Created on: 25 oct. 2022
 *      Author: Ludo
 */

#ifndef __LPTIM_H__
#define __LPTIM_H__

#include "types.h"

/*** LPTIM structures ***/

typedef enum {
	LPTIM_SUCCESS = 0,
	LPTIM_ERROR_DELAY_UNDERFLOW,
	LPTIM_ERROR_DELAY_OVERFLOW,
	LPTIM_ERROR_WRITE_ARR,
	LPTIM_ERROR_DELAY_MODE,
	LPTIM_ERROR_BASE_LAST = 0x0100
} LPTIM_status_t;

typedef enum {
	LPTIM_DELAY_MODE_ACTIVE = 0,
	LPTIM_DELAY_MODE_STOP,
	LPTIM_DELAY_MODE_LAST
} LPTIM_delay_mode_t;

/*** LPTIM functions ***/

void LPTIM1_init(uint32_t lsi_freq_hz);
LPTIM_status_t LPTIM1_delay_milliseconds(uint32_t delay_ms, LPTIM_delay_mode_t delay_mode);

LPTIM_status_t LPTIM1_start(uint32_t delay_ms);
void LPTIM1_stop(void);
uint8_t LPTIM1_get_wake_up_flag(void);

#define LPTIM1_check_status(error_base) { if (lptim1_status != LPTIM_SUCCESS) { status = error_base + lptim1_status; goto errors; }}
#define LPTIM1_stack_error() { ERROR_check_status(lptim1_status, LPTIM_SUCCESS, ERROR_BASE_LPTIM1); }
#define LPTIM1_stack_error_print() { ERROR_check_status_print(lptim1_status, LPTIM_SUCCESS, ERROR_BASE_LPTIM1); }

#endif /* __LPTIM_H__ */
