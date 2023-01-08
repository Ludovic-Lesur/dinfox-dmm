/*
 * tim.c
 *
 *  Created on: 22 aug. 2020
 *      Author: Ludo
 */

#include "tim.h"

#include "mapping.h"
#include "mode.h"
#include "nvic.h"
#include "rcc.h"
#include "rcc_reg.h"
#include "tim_reg.h"

/*** TIM local macros ***/

#define TIM_TIMEOUT_COUNT			1000000
#define TIM3_PWM_FREQUENCY_HZ		10000
#define TIM3_ARR_VALUE				((RCC_MSI_FREQUENCY_KHZ * 1000) / TIM3_PWM_FREQUENCY_HZ)
#define TIM3_NUMBER_OF_CHANNELS		4
#define TIM22_DIMMING_LUT_LENGTH	100

/*** TIM local structures ***/

typedef struct {
	volatile uint32_t dimming_lut_idx;
	volatile uint8_t dimming_lut_direction;
	volatile uint8_t single_blink_done;
} TIM22_context_t;

/*** TIM local global variables ***/

static const uint8_t TIM22_DIMMING_LUT[TIM22_DIMMING_LUT_LENGTH] = {
	211, 211, 211, 211, 211, 211, 211, 211, 210, 210,
	210, 210, 210, 210, 210, 210, 210, 209, 209, 209,
	209, 209, 209, 209, 208, 208, 208, 208, 207, 207,
	207, 207, 206, 206, 206, 205, 205, 205, 204, 204,
	203, 203, 202, 202, 201, 201, 200, 199, 199, 198,
	197, 196, 195, 194, 193, 192, 191, 190, 189, 188,
	186, 185, 183, 182, 180, 178, 176, 174, 172, 170,
	168, 165, 163, 160, 157, 154, 151, 148, 144, 140,
	136, 132, 127, 123, 118, 113, 107, 101, 95, 89,
	82, 74, 67, 59, 50, 41, 32, 22, 11, 0
};
static TIM22_context_t tim22_ctx;
static volatile uint8_t tim21_flag = 0;

/*** TIM local functions ***/

/* TIM22 INTERRUPT HANDLER.
 * @param:	None.
 * @return:	None.
 */
void __attribute__((optimize("-O0"))) TIM22_IRQHandler(void) {
	// Check update flag.
	if (((TIM22 -> SR) & (0b1 << 0)) != 0) {
		// Update duty cycles.
		TIM2 -> CCR1 = TIM22_DIMMING_LUT[tim22_ctx.dimming_lut_idx];
		TIM2 -> CCR2 = TIM22_DIMMING_LUT[tim22_ctx.dimming_lut_idx];
		TIM2 -> CCR3 = TIM22_DIMMING_LUT[tim22_ctx.dimming_lut_idx];
		// Manage index and direction.
		if (tim22_ctx.dimming_lut_direction == 0) {
			// Increment index.
			tim22_ctx.dimming_lut_idx++;
			// Invert direction at end of table.
			if (tim22_ctx.dimming_lut_idx >= (TIM22_DIMMING_LUT_LENGTH - 1)) {
				tim22_ctx.dimming_lut_direction = 1;
			}
		}
		else {
			// Decrement index.
			tim22_ctx.dimming_lut_idx--;
			// Invert direction at the beginning of table.
			if (tim22_ctx.dimming_lut_idx == 0) {
				// Single blink done.
				TIM3_stop();
				TIM22_stop();
				tim22_ctx.dimming_lut_direction = 0;
				tim22_ctx.single_blink_done = 1;
			}
		}
		// Clear flag.
		TIM22 -> SR &= ~(0b1 << 0);
	}
}

/*** TIM functions ***/

/* INIT TIM3 FOR PWM OPERATION.
 * @param:	None.
 * @return:	None.
 */
void TIM3_init(void) {
	// Enable peripheral clock.
	RCC -> APB1ENR |= (0b1 << 1); // TIM3EN='1'.
	// Set PWM frequency.
	TIM3 -> ARR = TIM3_ARR_VALUE; // Timer input clock is SYSCLK (PSC=0 by default).
	// Configure channels 1-4 in PWM mode 1 (OCxM='110' and OCxPE='1').
	TIM3 -> CCMR1 |= (0b110 << 12) | (0b1 << 11) | (0b110 << 4) | (0b1 << 3);
	TIM3 -> CCMR2 |= (0b110 << 12) | (0b1 << 11) | (0b110 << 4) | (0b1 << 3);
	// Disable all channels by default (CCxE='0').
	TIM3 -> CCR1 = (TIM3_ARR_VALUE + 1);
	TIM3 -> CCR2 = (TIM3_ARR_VALUE + 1);
	TIM3 -> CCR3 = (TIM3_ARR_VALUE + 1);
	// Generate event to update registers.
	TIM3 -> EGR |= (0b1 << 0); // UG='1'.
}

/* SET CURRENT LED COLOR.
 * @param led_color:	New LED color.
 * @return:				None.
 */
void TIM3_set_color_mask(TIM3_channel_mask_t led_color) {
	// Reset bits.
	TIM3 -> CCER &= 0xFFFFEEEE;
	// Enable channels according to color.
	uint8_t idx = 0;
	for (idx=0 ; idx<TIM3_NUMBER_OF_CHANNELS ; idx++) {
		if ((led_color & (0b1 << idx)) != 0) {
			TIM3 -> CCER |= (0b1 << (4 * idx));
		}
	}
}

/* START PWM GENERATION.
 * @param:	None.
 * @return:	None.
 */
void TIM3_start(void) {
	// Link GPIOs to timer.
	GPIO_configure(&GPIO_LED_RED, GPIO_MODE_ALTERNATE_FUNCTION, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_LED_GREEN, GPIO_MODE_ALTERNATE_FUNCTION, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_LED_BLUE, GPIO_MODE_ALTERNATE_FUNCTION, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	// Enable counter.
	TIM3 -> CNT = 0;
	TIM3 -> CR1 |= (0b1 << 0); // CEN='1'.
}

/* STOP PWM GENERATION.
 * @param:	None.
 * @return:	None.
 */
void TIM3_stop(void) {
	// Disable all channels.
	TIM3 -> CCER &= 0xFFFFEEEE;
	// Disable and reset counter.
	TIM3 -> CR1 &= ~(0b1 << 0); // CEN='0'.
}

/* INIT TIM22 FOR LED BLINKING OPERATION.
 * @param:	None.
 * @return:	None.
 */
void TIM22_init(void) {
	// Enable peripheral clock.
	RCC -> APB2ENR |= (0b1 << 5); // TIM22EN='1'.
	// Configure period.
	TIM22 -> PSC = 1; // Timer is clocked on (MSI / 2) .
	// Generate event to update registers.
	TIM22 -> EGR |= (0b1 << 0); // UG='1'.
	// Enable interrupt.
	TIM22 -> DIER |= (0b1 << 0);
	// Set interrupt priority.
	NVIC_set_priority(NVIC_INTERRUPT_TIM22, NVIC_PRIORITY_MIN);
}

/* START TIM22 PERIPHERAL.
 * @param led_blink_period_ms:	LED blink period in ms.
 * @return:						None.
 */
void TIM22_start(uint32_t led_blink_period_ms) {
	// Reset LUT index and flag.
	tim22_ctx.dimming_lut_idx = 0;
	tim22_ctx.dimming_lut_direction = 0;
	tim22_ctx.single_blink_done = 0;
	// Set period.
	TIM22 -> CNT = 0;
	TIM22 -> ARR = (led_blink_period_ms * RCC_MSI_FREQUENCY_KHZ) / (4 * TIM22_DIMMING_LUT_LENGTH);
	// Clear flag and enable interrupt.
	TIM22 -> SR &= ~(0b1 << 0); // Clear flag (UIF='0').
	NVIC_enable_interrupt(NVIC_INTERRUPT_TIM22);
	// Enable TIM22 peripheral.
	TIM22 -> CR1 |= (0b1 << 0); // Enable TIM22 (CEN='1').
}

/* STOP TIM22 COUNTER.
 * @param:	None.
 * @return:	None.
 */
void TIM22_stop(void) {
	// Disable interrupt.
	NVIC_disable_interrupt(NVIC_INTERRUPT_TIM22);
	// Stop TIM22.
	TIM22 -> CR1 &= ~(0b1 << 0); // CEN='0'.
}

/* GET SINGLE BLINK STATUS.
 * @param:						None.
 * @return single_blink_done:	'1' if the single blink is finished, '0' otherwise.
 */
uint8_t TIM22_is_single_blink_done(void) {
	return (tim22_ctx.single_blink_done);
}

/* CONFIGURE TIM21 FOR LSI FREQUENCY MEASUREMENT.
 * @param:	None.
 * @return:	None.
 */
void TIM21_init(void) {
	// Enable peripheral clock.
	RCC -> APB2ENR |= (0b1 << 2); // TIM21EN='1'.
	// Configure timer.
	// Channel input on TI1.
	// Capture done every 8 edges.
	// CH1 mapped on LSI.
	TIM21 -> CCMR1 |= (0b01 << 0) | (0b11 << 2);
	TIM21 -> OR |= (0b101 << 2);
	// Enable interrupt.
	TIM21 -> DIER |= (0b1 << 1); // CC1IE='1'.
	// Generate event to update registers.
	TIM21 -> EGR |= (0b1 << 0); // UG='1'.
}

/* MEASURE LSI CLOCK FREQUENCY WITH TIM21 CH1.
 * @param lsi_frequency_hz:		Pointer that will contain measured LSI frequency in Hz.
 * @return status:				Function execution status.
 */
TIM_status_t TIM21_get_lsi_frequency(uint32_t* lsi_frequency_hz) {
	// Local variables.
	TIM_status_t status = TIM_SUCCESS;
	uint8_t tim21_interrupt_count = 0;
	uint32_t tim21_ccr1_edge1 = 0;
	uint32_t tim21_ccr1_edge8 = 0;
	uint32_t loop_count = 0;
	// Check parameters.
	if (lsi_frequency_hz == NULL) {
		status = TIM_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Reset counter.
	TIM21 -> CNT = 0;
	TIM21 -> CCR1 = 0;
	// Enable interrupt.
	TIM21 -> SR &= 0xFFFFF9B8; // Clear all flags.
	NVIC_enable_interrupt(NVIC_INTERRUPT_TIM21);
	// Enable TIM21 peripheral.
	TIM21 -> CR1 |= (0b1 << 0); // CEN='1'.
	TIM21 -> CCER |= (0b1 << 0); // CC1E='1'.
	// Wait for 2 captures.
	while (tim21_interrupt_count < 2) {
		// Wait for interrupt.
		tim21_flag = 0;
		loop_count = 0;
		while (tim21_flag == 0) {
			loop_count++;
			if (loop_count > TIM_TIMEOUT_COUNT) {
				status = TIM_ERROR_INTERRUPT_TIMEOUT;
				goto errors;
			}
		}
		tim21_interrupt_count++;
		if (tim21_interrupt_count == 1) {
			tim21_ccr1_edge1 = (TIM21 -> CCR1);
		}
		else {
			tim21_ccr1_edge8 = (TIM21 -> CCR1);
		}
	}
	// Compute LSI frequency.
	(*lsi_frequency_hz) = (8 * RCC_HSI_FREQUENCY_KHZ * 1000) / (tim21_ccr1_edge8 - tim21_ccr1_edge1);
errors:
	// Disable interrupt.
	NVIC_disable_interrupt(NVIC_INTERRUPT_TIM21);
	// Stop counter.
	TIM21 -> CR1 &= ~(0b1 << 0); // CEN='0'.
	TIM21 -> CCER &= ~(0b1 << 0); // CC1E='0'.
	return status;
}

/* DISABLE TIM21 PERIPHERAL.
 * @param:	None.
 * @return:	None.
 */
void TIM21_disable(void) {
	// Disable TIM21 peripheral.
	TIM21 -> CR1 &= ~(0b1 << 0); // CEN='0'.
	RCC -> APB2ENR &= ~(0b1 << 2); // TIM21EN='0'.
}
