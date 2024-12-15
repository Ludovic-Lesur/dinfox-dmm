/*
 * main.c
 *
 *  Created on: 07 jan. 2023
 *      Author: Ludo
 */

// Peripherals.
#include "exti.h"
#include "gpio.h"
#include "iwdg.h"
#include "lptim.h"
#include "mapping.h"
#include "nvic.h"
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
// Applicative.
#include "error.h"
#include "hmi.h"
#include "mode.h"

/*** MAIN local macros ***/

#define DMM_POWER_ON_DELAY_MS	2000

/*** MAIN local functions ***/

/*******************************************************************/
static void _DMM_init_hw(void) {
	// Local variables.
	RCC_status_t rcc_status = RCC_SUCCESS;
	RTC_status_t rtc_status = RTC_SUCCESS;
	LED_status_t led_status = LED_SUCCESS;
#ifndef DEBUG
	IWDG_status_t iwdg_status = IWDG_SUCCESS;
#endif
	// Init error stack
	ERROR_stack_init();
	// Init memory.
	NVIC_init();
	// Init power module and clock tree.
	PWR_init();
	RCC_init();
	// Init GPIOs.
	GPIO_init();
	EXTI_init();
#ifndef DEBUG
	// Start independent watchdog.
	iwdg_status = IWDG_init();
	IWDG_stack_error();
	IWDG_reload();
#endif
	// High speed oscillator.
	rcc_status = RCC_switch_to_hsi();
	RCC_stack_error();
	// Calibrate clocks.
	rcc_status = RCC_calibrate();
	RCC_stack_error();
	// Init RTC.
	rtc_status = RTC_init();
	RTC_stack_error();
	// Init delay timer.
	LPTIM1_init();
	// Init components.
	POWER_init();
	led_status = LED_init();
	LED_stack_error();
	// Init nodes layer.
	NODE_init_por();
	// Init HMI wake-up control.
	HMI_init_por();
}

/*** MAIN functions ***/

/*******************************************************************/
int main(void) {
	// Init board.
	_DMM_init_hw();
	// Local variables.
	LPTIM_status_t lptim1_status = LPTIM_SUCCESS;
	NODE_status_t node_status = NODE_SUCCESS;
	HMI_status_t hmi_status = HMI_SUCCESS;
	// Power on delay to wait for slaves node startup.
	lptim1_status = LPTIM1_delay_milliseconds(DMM_POWER_ON_DELAY_MS, LPTIM_DELAY_MODE_STOP);
	LPTIM1_stack_error();
	// Perform first nodes scan.
	node_status = NODE_scan();
	NODE_stack_error();
	// Main loop.
	while (1) {
		// Enter sleep mode.
		IWDG_reload();
		PWR_enter_stop_mode();
		IWDG_reload();
		// Process HMI.
		hmi_status = HMI_process();
		HMI_stack_error();
		// Process nodes.
		NODE_process();
	}
	return 0;
}
