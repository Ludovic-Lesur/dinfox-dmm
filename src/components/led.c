/*
 * led.c
 *
 *  Created on: 22 aug. 2020
 *      Author: Ludo
 */

#include "led.h"

#include "gpio.h"
#include "mapping.h"
#include "tim.h"
#include "types.h"

/*** LED local functions ***/

/*******************************************************************/
static void _LED_off(void) {
	// Configure pins as output high.
	GPIO_write(&GPIO_LED_RED, 1);
	GPIO_write(&GPIO_LED_GREEN, 1);
	GPIO_write(&GPIO_LED_BLUE, 1);
	GPIO_configure(&GPIO_LED_RED, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_LED_GREEN, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_LED_BLUE, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
}

/*** LED functions ***/

/*******************************************************************/
LED_status_t LED_init(void) {
	// Local variables.
	LED_status_t status = LED_SUCCESS;
	TIM_status_t tim3_status = TIM_SUCCESS;
	// Init timers.
	tim3_status = TIM3_init();
	TIM3_exit_error(LED_ERROR_BASE_TIM3);
	TIM22_init();
errors:
	// Turn LED off.
	_LED_off();
	return status;
}

/*******************************************************************/
LED_status_t LED_start_single_blink(uint32_t blink_duration_ms, LED_color_t color) {
	// Local variables.
	LED_status_t status = LED_SUCCESS;
	TIM_status_t tim22_status = TIM_SUCCESS;
	// Check parameters.
	if (blink_duration_ms == 0) {
		status = LED_ERROR_NULL_DURATION;
		goto errors;
	}
	if (color >= LED_COLOR_LAST) {
		status = LED_ERROR_COLOR;
		goto errors;
	}
	// Link GPIOs to timer.
	GPIO_configure(&GPIO_LED_RED, GPIO_MODE_ALTERNATE_FUNCTION, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_LED_GREEN, GPIO_MODE_ALTERNATE_FUNCTION, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_LED_BLUE, GPIO_MODE_ALTERNATE_FUNCTION, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	// Start blink.
	TIM3_start(color);
	tim22_status = TIM22_start(blink_duration_ms);
	TIM22_exit_error(LED_ERROR_BASE_TIM22);
errors:
	return status;
}

/*******************************************************************/
uint8_t LED_is_single_blink_done(void) {
	return TIM22_is_single_blink_done();
}

/*******************************************************************/
void LED_stop_blink(void) {
	// Stop timers.
	TIM3_stop();
	TIM22_stop();
	// Turn LED off.
	_LED_off();
}
