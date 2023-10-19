/*
 * tim.h
 *
 *  Created on: 22 aug. 2020
 *      Author: Ludo
 */

#ifndef __TIM_H__
#define __TIM_H__

#include "rcc.h"
#include "types.h"

/*** TIM structures ***/

/*!******************************************************************
 * \enum TIM_status_t
 * \brief TIM driver error codes.
 *******************************************************************/
typedef enum {
	// Driver errors.
	TIM_SUCCESS = 0,
	TIM_ERROR_NULL_PARAMETER,
	TIM_ERROR_INTERRUPT_TIMEOUT,
	TIM_ERROR_CHANNEL,
	TIM_ERROR_DURATION_UNDERFLOW,
	TIM_ERROR_DURATION_OVERFLOW,
	TIM_ERROR_WAITING_MODE,
	TIM_ERROR_COMPLETION_WATCHDOG,
	// Low level drivers errors.
	TIM_ERROR_BASE_RCC = 0x0100,
	// Last base value.
	TIM_ERROR_BASE_LAST = (TIM_ERROR_BASE_RCC + RCC_ERROR_BASE_LAST)
} TIM_status_t;

/*!******************************************************************
 * \enum TIM2_channel_t
 * \brief TIM channels list.
 *******************************************************************/
typedef enum {
	TIM2_CHANNEL_1 = 0,
	TIM2_CHANNEL_2,
	TIM2_CHANNEL_3,
	TIM2_CHANNEL_4,
	TIM2_CHANNEL_LAST
} TIM2_channel_t;

/*!******************************************************************
 * \enum TIM_waiting_mode_t
 * \brief Timer completion waiting modes.
 *******************************************************************/
typedef enum {
	TIM_WAITING_MODE_ACTIVE = 0,
	TIM_WAITING_MODE_SLEEP,
	TIM_WAITING_MODE_LOW_POWER_SLEEP,
	TIM_WAITING_MODE_LAST
} TIM_waiting_mode_t;

/*!******************************************************************
 * \enum TIM3_channel_t
 * \brief TIM channels mapping defined as (channel number - 1).
 *******************************************************************/
typedef enum {
	TIM3_CHANNEL_LED_RED = 1,
	TIM3_CHANNEL_LED_GREEN = 2,
	TIM3_CHANNEL_LED_BLUE = 3
} TIM3_channel_t;

/*!******************************************************************
 * \enum TIM3_channel_mask_t
 * \brief LED color bit mask defined as 0b<CH4><CH3><CH2><CH1>.
 *******************************************************************/
typedef enum {
	TIM3_CHANNEL_MASK_OFF = 0b0000,
	TIM3_CHANNEL_MASK_RED = (0b1 << TIM3_CHANNEL_LED_RED),
	TIM3_CHANNEL_MASK_GREEN = (0b1 << TIM3_CHANNEL_LED_GREEN),
	TIM3_CHANNEL_MASK_YELLOW = (0b1 << TIM3_CHANNEL_LED_RED) | (0b1 << TIM3_CHANNEL_LED_GREEN),
	TIM3_CHANNEL_MASK_BLUE = (0b1 << TIM3_CHANNEL_LED_BLUE),
	TIM3_CHANNEL_MASK_MAGENTA = (0b1 << TIM3_CHANNEL_LED_RED) | (0b1 << TIM3_CHANNEL_LED_BLUE),
	TIM3_CHANNEL_MASK_CYAN = (0b1 << TIM3_CHANNEL_LED_GREEN) | (0b1 << TIM3_CHANNEL_LED_BLUE),
	TIM3_CHANNEL_MASK_WHITE	= (0b1 << TIM3_CHANNEL_LED_RED) | (0b1 << TIM3_CHANNEL_LED_GREEN) | (0b1 << TIM3_CHANNEL_LED_BLUE)
} TIM3_channel_mask_t;

/*** TIM functions ***/

/*!******************************************************************
 * \fn void TIM2_init(void)
 * \brief Init TIM2 peripheral for general purpose timer operation.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		none
 *******************************************************************/
void TIM2_init(void);

/*!******************************************************************
 * \fn void TIM2_de_init(void)
 * \brief Release TIM2 peripheral.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		none
 *******************************************************************/
void TIM2_de_init(void);

/*!******************************************************************
 * \fn TIM_status_t TIM2_start(TIM2_channel_t channel, uint32_t duration_ms)
 * \brief Start a timer channel.
 * \param[in]  	channel: Channel to start.
 * \param[in]	duration_ms: Timer duration in ms.
 * \param[in]	waiting_mode: Completion waiting mode.
 * \param[out] 	none
 * \retval		Function execution status.
 *******************************************************************/
TIM_status_t TIM2_start(TIM2_channel_t channel, uint32_t duration_ms, TIM_waiting_mode_t waiting_mode);

/*!******************************************************************
 * \fn TIM_status_t TIM2_stop(TIM2_channel_t channel)
 * \brief Stop a timer channel.
 * \param[in]  	channel: Channel to stop.
 * \param[out] 	none
 * \retval		Function execution status.
 *******************************************************************/
TIM_status_t TIM2_stop(TIM2_channel_t channel);

/*!******************************************************************
 * \fn TIM_status_t TIM2_get_status(TIM2_channel_t channel, uint8_t* timer_has_elapsed)
 * \brief Get the status of a timer channel.
 * \param[in]  	channel: Channel to read.
 * \param[out]	timer_has_elapsed: Pointer to bit that will contain the timer status (0 for running, 1 for complete).
 * \retval		Function execution status.
 *******************************************************************/
TIM_status_t TIM2_get_status(TIM2_channel_t channel, uint8_t* timer_has_elapsed);

/*!******************************************************************
 * \fn TIM_status_t TIM2_wait_completion(TIM2_channel_t channel)
 * \brief Blocking function waiting for a timer channel completion.
 * \param[in]  	channel: Channel to wait for.
 * \param[in]	waiting_mode: Completion waiting mode.
 * \param[out] 	none
 * \retval		Function execution status.
 *******************************************************************/
TIM_status_t TIM2_wait_completion(TIM2_channel_t channel, TIM_waiting_mode_t waiting_mode);

/*!******************************************************************
 * \fn void TIM3_init(void)
 * \brief Init TIM3 peripheral for RGB LED blinking operation.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		none
 *******************************************************************/
void TIM3_init(void);

/*!******************************************************************
 * \fn void TIM3_start(TIM3_channel_mask_t led_color)
 * \brief Start PWM signal for a given color.
 * \param[in]  	led_color: LED color.
 * \param[out] 	none
 * \retval		none
 *******************************************************************/
void TIM3_start(TIM3_channel_mask_t led_color);

/*!******************************************************************
 * \fn void TIM3_stop(void)
 * \brief Stop PWM signal.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		none
 *******************************************************************/
void TIM3_stop(void);

/*!******************************************************************
 * \fn void TIM21_init(void)
 * \brief Init TIM21 peripheral for RGB LED blinking operation.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		none
 *******************************************************************/
void TIM21_init(void);

/*!******************************************************************
 * \fn void TIM21_start(uint32_t led_blink_period_ms)
 * \brief Start LED blink duration timer.
 * \param[in]  	led_blink_period_ms: Blink duration in ms.
 * \param[out] 	none
 * \retval		none
 *******************************************************************/
void TIM21_start(uint32_t led_blink_period_ms);

/*!******************************************************************
 * \fn void TIM21_stop(void)
 * \brief Stop LED blink timer.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		none
 *******************************************************************/
void TIM21_stop(void);

/*!******************************************************************
 * \fn uint8_t TIM21_is_single_blink_done(void)
 * \brief Get the LED blink status.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		0 if the LED blink id running, 1 if it is complete.
 *******************************************************************/
uint8_t TIM21_is_single_blink_done(void);

/*******************************************************************/
#define TIM2_exit_error(error_base) { if (tim2_status != TIM_SUCCESS) { status = (error_base + tim2_status); goto errors; } }

/*******************************************************************/
#define TIM2_stack_error(void) { if (tim2_status != TIM_SUCCESS) { ERROR_stack_add(ERROR_BASE_TIM2 + tim2_status); } }

/*******************************************************************/
#define TIM2_stack_exit_error(error_code) { if (tim2_status != TIM_SUCCESS) { ERROR_stack_add(ERROR_BASE_TIM2 + tim2_status); status = error_code; goto errors; } }

#endif /* __TIM_H__ */
