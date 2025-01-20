/*
 * main.c
 *
 *  Created on: 07 jan. 2023
 *      Author: Ludo
 */

// Peripherals.
#include "exti.h"
#include "gpio.h"
#include "gpio_mapping.h"
#include "iwdg.h"
#include "lptim.h"
#include "nvic.h"
#include "nvic_priority.h"
#include "pwr.h"
#include "rcc.h"
#include "rtc.h"
// Utils.
#include "types.h"
// Components.
#include "led.h"
#include "power.h"
// Nodes.
#include "node.h"
// Middleware.
#include "hmi.h"
#include "radio.h"
// Applicative.
#include "dmm_flags.h"
#include "error.h"
#include "error_base.h"

/*** MAIN local macros ***/

#define DMM_POWER_ON_DELAY_MS	2000

/*** MAIN local functions ***/

/*******************************************************************/
static void _DMM_init_hw(void) {
	// Local variables.
	RCC_status_t rcc_status = RCC_SUCCESS;
	RTC_status_t rtc_status = RTC_SUCCESS;
	LED_status_t led_status = LED_SUCCESS;
    NODE_status_t node_status = NODE_SUCCESS;
    HMI_status_t hmi_status = HMI_SUCCESS;
    RADIO_status_t radio_status = RADIO_SUCCESS;
#ifndef DMM_DEBUG
	IWDG_status_t iwdg_status = IWDG_SUCCESS;
#endif
	// Init error stack
	ERROR_stack_init();
	// Init memory.
	NVIC_init();
	// Init power module and clock tree.
	PWR_init();
	rcc_status = RCC_init(NVIC_PRIORITY_CLOCK);
    RCC_stack_error(ERROR_BASE_RCC);
	// Init GPIOs.
	GPIO_init();
	EXTI_init();
#ifndef DMM_DEBUG
	// Start independent watchdog.
	iwdg_status = IWDG_init();
	IWDG_stack_error(ERROR_BASE_IWDG);
	IWDG_reload();
#endif
	// High speed oscillator.
	rcc_status = RCC_switch_to_hsi();
	RCC_stack_error(ERROR_BASE_RCC);
	// Calibrate clocks.
	rcc_status = RCC_calibrate_internal_clocks(NVIC_PRIORITY_CLOCK_CALIBRATION);
	RCC_stack_error(ERROR_BASE_RCC);
	// Init RTC.
	rtc_status = RTC_init(NULL, NVIC_PRIORITY_RTC);
	RTC_stack_error(ERROR_BASE_RTC);
	// Init delay timer.
	LPTIM_init(NVIC_PRIORITY_DELAY);
	// Init components.
	POWER_init();
	led_status = LED_init();
	LED_stack_error(ERROR_BASE_LED);
	// Init nodes layer.
	node_status = NODE_init();
	NODE_stack_error(ERROR_BASE_NODE);
	// Init HMI wake-up control.
	hmi_status = HMI_init_por();
	HMI_stack_error(ERROR_BASE_HMI);
	// Init radio.
	radio_status = RADIO_init();
	RADIO_stack_error(ERROR_BASE_RADIO);
}

/*** MAIN functions ***/

/*******************************************************************/
int main(void) {
	// Init board.
	_DMM_init_hw();
	// Local variables.
	LPTIM_status_t lptim_status = LPTIM_SUCCESS;
	NODE_status_t node_status = NODE_SUCCESS;
	HMI_status_t hmi_status = HMI_SUCCESS;
	RADIO_status_t radio_status = RADIO_SUCCESS;
	// Power on delay to wait for slaves node startup.
	lptim_status = LPTIM_delay_milliseconds(DMM_POWER_ON_DELAY_MS, LPTIM_DELAY_MODE_STOP);
	LPTIM_stack_error(ERROR_BASE_LPTIM);
	// Perform first nodes scan.
	node_status = NODE_scan();
	NODE_stack_error(ERROR_BASE_NODE);
	// Main loop.
	while (1) {
		// Enter sleep mode.
		IWDG_reload();
#ifndef DMM_DEBUG
		PWR_enter_stop_mode();
		IWDG_reload();
#endif
		// Process nodes.
        node_status = NODE_process();
        NODE_stack_error(ERROR_BASE_NODE);
		// Process HMI.
		hmi_status = HMI_process();
		HMI_stack_error(ERROR_BASE_HMI);
		// Process radio.
		radio_status = RADIO_process();
        RADIO_stack_error(ERROR_BASE_RADIO);
	}
	return 0;
}
