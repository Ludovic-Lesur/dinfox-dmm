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
	LED_ERROR_BASE_TIM3 = 0x0100,
	LED_ERROR_BASE_TIM22 = (LED_ERROR_BASE_TIM3 + TIM_ERROR_BASE_LAST),
	// Last base value.
	LED_ERROR_BASE_LAST = (LED_ERROR_BASE_TIM22 + TIM_ERROR_BASE_LAST)
} LED_status_t;

/*!******************************************************************
 * \enum LED_color_t
 * \brief LED colors list.
 *******************************************************************/
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

/*!******************************************************************
 * \fn LED_status_t LED_init(void)
 * \brief Init LED driver.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		Function execution status.
 *******************************************************************/
LED_status_t LED_init(void);

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
 * \fn uint8_t LED_is_single_blink_done(void)
 * \brief Get blink status.
 * \param[in]  	none
 * \param[in]	none
 * \param[out] 	none
 * \retval		1 of the LED blink is complete, 0 otherwise.
 *******************************************************************/
uint8_t LED_is_single_blink_done(void);

/*!******************************************************************
 * \fn void LED_stop_blink(void)
 * \brief Stop LED blink.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		none
 *******************************************************************/
void LED_stop_blink(void);

/*******************************************************************/
#define LED_exit_error(error_base) { if (led_status != LED_SUCCESS) { status = (error_base + led_status); goto errors; } }

/*******************************************************************/
#define LED_stack_error(void) { if (led_status != LED_SUCCESS) { ERROR_stack_add(ERROR_BASE_LED + led_status); } }

/*******************************************************************/
#define LED_stack_exit_error(error_code) { if (led_status != LED_SUCCESS) { ERROR_stack_add(ERROR_BASE_LED + led_status); status = error_code; goto errors; } }

#endif /* LED_H */
