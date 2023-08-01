/*
 * flash.h
 *
 *  Created on: 31 march 2019
 *      Author: Ludo
 */

#ifndef __FLASH_H__
#define __FLASH_H__

#include "types.h"

/*** FLASH structures ***/

typedef enum {
	FLASH_SUCCESS = 0,
	FLASH_ERROR_LATENCY,
	FLASH_ERROR_TIMEOUT,
	FLASH_ERROR_BASE_LAST = 0x0100
} FLASH_status_t;

/*** FLASH functions ***/

FLASH_status_t FLASH_set_latency(uint8_t wait_states);

#define FLASH_check_status(error_base) { if (flash_status != FLASH_SUCCESS) { status = error_base + flash_status; goto errors; }}
#define FLASH_stack_error() { ERROR_check_status(flash_status, FLASH_SUCCESS, ERROR_BASE_FLASH); }
#define FLASH_stack_error_print() { ERROR_check_status_print(flash_status, FLASH_SUCCESS, ERROR_BASE_FLASH); }

#endif /* __FLASH_H__ */
