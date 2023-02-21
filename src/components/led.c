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

/*** LED functions ***/

/* INIT LED.
 * @param:	None.
 * @return:	None.
 */
void LED_init(void) {
	// Configure pins as output high.
	GPIO_write(&GPIO_LED_RED, 1);
	GPIO_write(&GPIO_LED_GREEN, 1);
	GPIO_write(&GPIO_LED_BLUE, 1);
	GPIO_configure(&GPIO_LED_RED, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_LED_GREEN, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_LED_BLUE, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
}

/* SET STATIC LED COLOR.
 * @param led_color:	Color to set.
 * @return status:		Function execution status.
 */
LED_status_t LED_set_color(LED_color_t color) {
	// Local variables.
	LED_status_t status = LED_SUCCESS;
	// Turn off by default.
	LED_init();
	// Check color.
	switch (color) {
	case LED_COLOR_OFF:
		// Nothing to do.
		break;
	case LED_COLOR_RED:
		GPIO_write(&GPIO_LED_RED, 0);
		break;
	case LED_COLOR_GREEN:
		GPIO_write(&GPIO_LED_GREEN, 0);
		break;
	case LED_COLOR_YELLOW:
		GPIO_write(&GPIO_LED_RED, 0);
		GPIO_write(&GPIO_LED_GREEN, 0);
		break;
	case LED_COLOR_BLUE:
		GPIO_write(&GPIO_LED_BLUE, 0);
		break;
	case LED_COLOR_MAGENTA:
		GPIO_write(&GPIO_LED_RED, 0);
		GPIO_write(&GPIO_LED_BLUE, 0);
		break;
	case LED_COLOR_CYAN:
		GPIO_write(&GPIO_LED_GREEN, 0);
		GPIO_write(&GPIO_LED_BLUE, 0);
		break;
	case LED_COLOR_WHITE:
		GPIO_write(&GPIO_LED_RED, 0);
		GPIO_write(&GPIO_LED_GREEN, 0);
		GPIO_write(&GPIO_LED_BLUE, 0);
		break;
	default:
		status = LED_ERROR_COLOR;
		goto errors;
	}
errors:
	return status;
}

/* START A SINGLE LED BLINK.
 * @param led_color:		Color to set.
 * @param blink_period_ms:	Blink duration in ms.
 * @return status:			Function execution status.
 */
LED_status_t LED_start_single_blink(LED_color_t color, uint32_t blink_duration_ms) {
	// Local variables.
	LED_status_t status = LED_SUCCESS;
	// Check parameters.
	if (blink_duration_ms == 0) {
		status = LED_ERROR_NULL_DURATION;
		goto errors;
	}
	if (color >= LED_COLOR_LAST) {
		status = LED_ERROR_COLOR;
		goto errors;
	}
	// Start blink.
	TIM3_start(color);
	TIM22_start(blink_duration_ms);
errors:
	return status;
}

/* CHECK LED BLINK STATUS.
 * @param:	None.
 * @return:	'1' if the blink is done, '0' otherwise.
 */
uint8_t LED_is_single_blink_done(void) {
	return TIM22_is_single_blink_done();
}

/* STOP LED BLINK.
 * @param:	None.
 * @return:	None.
 */
void LED_stop_blink(void) {
	// Stop timers.
	TIM3_stop();
	TIM22_stop();
	// Turn LED off.
	LED_init();
}
