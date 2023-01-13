/*
 * main.c
 *
 *  Created on: Jan 7, 2023
 *      Author: Ludo
 */

// Registers
#include "rcc_reg.h"
// Peripherals.
#include "adc.h"
#include "exti.h"
#include "flash.h"
#include "gpio.h"
#include "i2c.h"
#include "iwdg.h"
#include "lptim.h"
#include "lpuart.h"
#include "mapping.h"
#include "nvic.h"
#include "nvm.h"
#include "pwr.h"
#include "rcc.h"
#include "rtc.h"
// Components.
#include "led.h"
#include "rs485.h"
#include "rs485_common.h"
#include "sh1106.h"
// Utils.
#include "logo.h"
#include "types.h"
// Applicative.
#include "error.h"
#include "mode.h"
#include "version.h"

/*** MAIN local macros ***/

/*** MAIN local structures ***/

typedef union {
	struct {
		unsigned lse_status : 1;
		unsigned lsi_status : 1;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
	uint8_t all;
} DIM_status_t;

typedef enum {
	DMM_STATE_INIT,
	DMM_STATE_HMI,
	DMM_STATE_OFF,
	DMM_STATE_SLEEP,
	DMM_STATE_LAST,
} DMM_state_t;

typedef struct {
	// Global.
	DMM_state_t state;
	DIM_status_t status;
	// Clocks.
	uint32_t lsi_frequency_hz;
	uint8_t lse_running;
} DMM_context_t;

/*** MAIN local global variables ***/

static DMM_context_t dmm_ctx;

/*** MAIN local functions ***/

/* COMMON INIT FUNCTION FOR MAIN CONTEXT.
 * @param:	None.
 * @return:	None.
 */
void _DMM_init_context(void) {
	// Init context.
	dmm_ctx.state = DMM_STATE_INIT;
	dmm_ctx.lsi_frequency_hz = 0;
	dmm_ctx.lse_running = 0;
	dmm_ctx.status.all = 0;
}

/* COMMON INIT FUNCTION FOR PERIPHERALS AND COMPONENTS.
 * @param:	None.
 * @return:	None.
 */
void _DMM_init_hw(void) {
	// Local variables.
	RCC_status_t rcc_status = RCC_SUCCESS;
	RTC_status_t rtc_status = RTC_SUCCESS;
	ADC_status_t adc1_status = ADC_SUCCESS;
#ifdef AM
	LPUART_status_t lpuart1_status = LPUART_SUCCESS;
	NVM_status_t nvm_status = NVM_SUCCESS;
	RS485_address_t node_address;
#endif
#ifndef DEBUG
	IWDG_status_t iwdg_status = IWDG_SUCCESS;
#endif
	// Init error stack
	ERROR_stack_init();
	// Init memory.
	NVIC_init();
	NVM_init();
	// Init GPIOs.
	GPIO_init();
	EXTI_init();
	// Init clock and power modules.
	RCC_init();
	PWR_init();
	// Reset RTC.
	RTC_reset();
	// Start oscillators.
	rcc_status = RCC_enable_lsi();
	RCC_error_check();
	dmm_ctx.status.lsi_status = (rcc_status == RCC_SUCCESS) ? 1 : 0;
	rcc_status = RCC_enable_lse();
	RCC_error_check();
	dmm_ctx.status.lse_status = (rcc_status == RCC_SUCCESS) ? 1 : 0;
	// Start independent watchdog.
#ifndef DEBUG
	iwdg_status = IWDG_init();
	IWDG_error_check();
#endif
	// High speed oscillator.
	IWDG_reload();
	rcc_status = RCC_switch_to_hsi();
	RCC_error_check();
	// Get LSI effective frequency (must be called after HSI initialization and before RTC inititialization).
	rcc_status = RCC_get_lsi_frequency(&dmm_ctx.lsi_frequency_hz);
	RCC_error_check();
	if (rcc_status != RCC_SUCCESS) dmm_ctx.lsi_frequency_hz = RCC_LSI_FREQUENCY_HZ;
	IWDG_reload();
	// RTC.
	dmm_ctx.lse_running = dmm_ctx.status.lse_status;
	rtc_status = RTC_init(&dmm_ctx.lse_running, dmm_ctx.lsi_frequency_hz);
	RTC_error_check();
	// Update LSE status if RTC failed to start on it.
	if (dmm_ctx.lse_running == 0) {
		dmm_ctx.status.lse_status = 0;
	}
	IWDG_reload();
#ifdef AM
	// Read RS485 address in NVM.
	nvm_status = NVM_read_byte(NVM_ADDRESS_RS485_ADDRESS, &node_address);
	NVM_error_check();
#endif
	// Init peripherals.
	LPTIM1_init(dmm_ctx.lsi_frequency_hz);
	TIM3_init();
	TIM22_init();
	adc1_status = ADC1_init();
	ADC1_error_check();
#ifdef AM
	lpuart1_status = LPUART1_init(node_address);
	LPUART1_error_check();
#else
	LPUART1_init();
#endif
	I2C1_init();
	// Init components.
	LED_init();
	RS485_init();
}

/*** MAIN functions ***/

/* MAIN FUNCTION.
 * @param:	None.
 * @return:	None.
 */
int main(void) {
	// Init board.
	_DMM_init_context();
	_DMM_init_hw();
	// Local variables.
	LED_status_t led_status = LED_SUCCESS;
	LED_color_t led_color = LED_COLOR_RED;
	// OLED test.
	I2C1_power_on();
	SH1106_init();
	SH1106_print_image(DINFOX_LOGO);
	// Main loop.
	while (1) {
		led_status = LED_start_single_blink(2000, led_color);
		LED_error_check();
		while (LED_is_single_blink_done() == 0);
		LED_stop_blink();
		switch (led_color) {
		case LED_COLOR_RED:
			led_color = LED_COLOR_GREEN;
			break;
		case LED_COLOR_GREEN:
			led_color = LED_COLOR_BLUE;
			break;
		case LED_COLOR_BLUE:
			led_color = LED_COLOR_RED;
			break;
		default:
			led_color = LED_COLOR_WHITE;
			break;
		}
		LPTIM1_delay_milliseconds(1000, 1);
		IWDG_reload();
	}
	return 0;
}
