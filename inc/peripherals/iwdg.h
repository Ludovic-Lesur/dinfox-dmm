/*
 * iwdg.h
 *
 *  Created on: 18 apr. 2020
 *      Author: Ludo
 */

#ifndef __IWDG_H__
#define __IWDG_H__

/*** IWDG structures ***/

/*!******************************************************************
 * \enum IWDG_status_t
 * \brief IWDG driver error codes.
 *******************************************************************/
typedef enum {
	IWDG_SUCCESS = 0,
	IWDG_ERROR_TIMEOUT,
	IWDG_ERROR_BASE_LAST = 0x0100
} IWDG_status_t;

/*** IWDG functions ***/

/*!******************************************************************
 * \fn IWDG_status_t IWDG_init(void)
 * \brief Start independent watchdog.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		Function execution status.
 *******************************************************************/
IWDG_status_t IWDG_init(void);

/*!******************************************************************
 * \fn void IWDG_reload(void)
 * \brief Refresh independent watchdog.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		Function execution status.
 *******************************************************************/
void IWDG_reload(void);

/*******************************************************************/
#define IWDG_check_status(error_base) { if (iwdg_status != IWDG_SUCCESS) { status = error_base + iwdg_status; goto errors; } }

/*******************************************************************/
#define IWDG_stack_error(void) { ERROR_stack_error(iwdg_status, IWDG_SUCCESS, ERROR_BASE_IWDG); }

/*******************************************************************/
#define IWDG_print_error(void) { ERROR_print_error(iwdg_status, IWDG_SUCCESS, ERROR_BASE_IWDG); }

#endif /* __IWDG_H__ */
