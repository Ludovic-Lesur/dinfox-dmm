/*
 * iwdg.h
 *
 *  Created on: 18 apr. 2020
 *      Author: Ludo
 */

#ifndef __IWDG_H__
#define __IWDG_H__

/*** IWDG macros ***/

#define IWDG_REFRESH_PERIOD_SECONDS		10

/*** IWDG structures ***/

typedef enum {
	IWDG_SUCCESS = 0,
	IWDG_ERROR_TIMEOUT,
	IWDG_ERROR_BASE_LAST = 0x0100
} IWDG_status_t;

/*** IWDG functions ***/

IWDG_status_t IWDG_init(void);
void IWDG_reload(void);

#define IWDG_check_status(error_base) { if (iwdg_status != IWDG_SUCCESS) { status = error_base + iwdg_status; goto errors; }}
#define IWDG_stack_error() { ERROR_check_status(iwdg_status, IWDG_SUCCESS, ERROR_BASE_IWDG); }
#define IWDG_stack_error_print() { ERROR_check_status_print(iwdg_status, IWDG_SUCCESS, ERROR_BASE_IWDG); }

#endif /* __IWDG_H__ */
