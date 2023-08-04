/*
 * flash.h
 *
 *  Created on: 16 aug. 2019
 *      Author: Ludo
 */

#ifndef __FLASH_H__
#define __FLASH_H__

#include "types.h"

/*** FLASH structures ***/

/*!******************************************************************
 * \enum FLASH_status_t
 * \brief FLASH driver error codes.
 *******************************************************************/
typedef enum {
	FLASH_SUCCESS = 0,
	FLASH_ERROR_LATENCY,
	FLASH_ERROR_TIMEOUT,
	FLASH_ERROR_BASE_LAST = 0x0100
} FLASH_status_t;

/*** FLASH functions ***/

/*!******************************************************************
 * \fn FLASH_status_t FLASH_set_latency(uint8_t wait_states)
 * \brief Set FLASH latency.
 * \param[in]  	wait_states: Number of wait states to set.
 * \param[out] 	none
 * \retval		Function execution status.
 *******************************************************************/
FLASH_status_t FLASH_set_latency(uint8_t wait_states);

/*******************************************************************/
#define FLASH_check_status(error_base) { if (flash_status != FLASH_SUCCESS) { status = error_base + flash_status; goto errors; } }

/*******************************************************************/
#define FLASH_stack_error(void) { ERROR_stack_error(flash_status, FLASH_SUCCESS, ERROR_BASE_FLASH); }

/*******************************************************************/
#define FLASH_print_error(void) { ERROR_print_error(flash_status, FLASH_SUCCESS, ERROR_BASE_FLASH); }

#endif /* __FLASH_H__ */
