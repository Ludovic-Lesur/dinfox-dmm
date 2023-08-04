/*
 * lptim.h
 *
 *  Created on: 29 apr. 2018
 *      Author: Ludo
 */

#ifndef __LPTIM_H__
#define __LPTIM_H__

#include "types.h"

/*** LPTIM structures ***/

/*!******************************************************************
 * \enum LPTIM_status_t
 * \brief LPTIM driver error codes.
 *******************************************************************/
typedef enum {
	LPTIM_SUCCESS = 0,
	LPTIM_ERROR_DELAY_UNDERFLOW,
	LPTIM_ERROR_DELAY_OVERFLOW,
	LPTIM_ERROR_WRITE_ARR,
	LPTIM_ERROR_DELAY_MODE,
	LPTIM_ERROR_BASE_LAST = 0x0100
} LPTIM_status_t;

/*!******************************************************************
 * \enum LPTIM_delay_mode_t
 * \brief LPTIM delay waiting modes.
 *******************************************************************/
typedef enum {
	LPTIM_DELAY_MODE_ACTIVE = 0,
	LPTIM_DELAY_MODE_SLEEP,
	LPTIM_DELAY_MODE_STOP,
	LPTIM_DELAY_MODE_LAST
} LPTIM_delay_mode_t;

/*** LPTIM functions ***/

/*!******************************************************************
 * \fn void LPTIM1_init(void)
 * \brief Init LPTIM1 peripheral for delay operation.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		none
 *******************************************************************/
void LPTIM1_init(void);

/*!******************************************************************
 * \fn LPTIM_status_t LPTIM1_delay_milliseconds(uint32_t delay_ms, LPTIM_delay_mode_t delay_mode)
 * \brief Delay function.
 * \param[in]  	delay_ms: Delay to wait in ms.
 * \param[in]	delay_mode: Delay waiting mode.
 * \param[out] 	none
 * \retval		Function execution status.
 *******************************************************************/
LPTIM_status_t LPTIM1_delay_milliseconds(uint32_t delay_ms, LPTIM_delay_mode_t delay_mode);

/*******************************************************************/
#define LPTIM1_check_status(error_base) { if (lptim1_status != LPTIM_SUCCESS) { status = error_base + lptim1_status; goto errors; } }

/*******************************************************************/
#define LPTIM1_stack_error(void) { ERROR_stack_error(lptim1_status, LPTIM_SUCCESS, ERROR_BASE_LPTIM1); }

/*******************************************************************/
#define LPTIM1_print_error(void) { ERROR_print_error(lptim1_status, LPTIM_SUCCESS, ERROR_BASE_LPTIM1); }

#endif /* __LPTIM_H__ */
