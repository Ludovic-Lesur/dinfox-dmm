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

/*!******************************************************************
 * \enum LED_status_t
 * \brief LED driver error codes.
 *******************************************************************/
typedef enum {
	// Driver errors.
	LED_SUCCESS,
	LED_ERROR_NULL_DURATION,
	LED_ERROR_COLOR,
	// Low level drivers errors.
	LED_ERROR_BASE_TIM_PWM = 0x0100,
	LED_ERROR_BASE_TIM_DIMMING = (LED_ERROR_BASE_TIM_PWM + TIM_ERROR_BASE_LAST),
	// Last base value.
	LED_ERROR_BASE_LAST = (LED_ERROR_BASE_TIM_DIMMING + TIM_ERROR_BASE_LAST)
} LED_status_t;

/*!******************************************************************
 * \enum LED_color_t
 * \brief LED colors list.
 *******************************************************************/
typedef enum {
	LED_COLOR_OFF = 0,
	LED_COLOR_RED,
	LED_COLOR_GREEN,
	LED_COLOR_YELLOW,
	LED_COLOR_BLUE,
	LED_COLOR_MAGENTA,
	LED_COLOR_CYAN,
	LED_COLOR_WHITE,
	LED_COLOR_LAST
} LED_color_t;

/*** LED functions ***/

/*!******************************************************************
 * \fn LED_status_t LED_init(void)
 * \brief Init LED driver.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		Function execution status.
 *******************************************************************/
LED_status_t LED_init(void);

/*!******************************************************************
 * \fn LED_status_t LED_de_init(void)
 * \brief Release LED driver.
 * \param[in]   none
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
LED_status_t LED_de_init(void);

/*!******************************************************************
 * \fn LED_status_t LED_start_single_blink(uint32_t blink_duration_ms, LED_color_t color)
 * \brief Start single blink.
 * \param[in]  	blink_duration_ms: Blink duration in ms.
 * \param[in]	color: LED color.
 * \param[out] 	none
 * \retval		Function execution status.
 *******************************************************************/
LED_status_t LED_start_single_blink(uint32_t blink_duration_ms, LED_color_t color);

/*!******************************************************************
 * \fn LED_status_t LED_stop_blink(void)
 * \brief Stop LED blink.
 * \param[in]   none
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
LED_status_t LED_stop_blink(void);

/*!******************************************************************
 * \fn uint8_t LED_is_single_blink_done(void)
 * \brief Get blink status.
 * \param[in]  	none
 * \param[in]	none
 * \param[out] 	none
 * \retval		1 of the LED blink is complete, 0 otherwise.
 *******************************************************************/
uint8_t LED_is_single_blink_done(void);

/*******************************************************************/
#define LED_exit_error(base) { ERROR_check_exit(led_status, LED_SUCCESS, base) }

/*******************************************************************/
#define LED_stack_error(base) { ERROR_check_stack(led_status, LED_SUCCESS, base) }

/*******************************************************************/
#define LED_stack_exit_error(base, code) { ERROR_check_stack_exit(led_status, LED_SUCCESS, base, code) }

#endif /* LED_H */
