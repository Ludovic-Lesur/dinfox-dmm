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

#if (defined LVRM) || (defined DDRM) || (defined RRM)

/*** TIM local macros ***/

#define TIM2_PWM_FREQUENCY_HZ		10000
#define TIM2_ARR_VALUE				((RCC_MSI_FREQUENCY_KHZ * 1000) / TIM2_PWM_FREQUENCY_HZ)
#define TIM2_NUMBER_OF_CHANNELS		4
#define TIM21_DIMMING_LUT_LENGTH	100

/*** TIM local structures ***/

typedef struct {
	volatile uint32_t dimming_lut_idx;
	volatile uint8_t dimming_lut_direction;
	volatile uint8_t single_blink_done;
} TIM21_context_t;

/*** TIM local global variables ***/

static const uint8_t TIM21_DIMMING_LUT[TIM21_DIMMING_LUT_LENGTH] = {
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
static TIM21_context_t tim21_ctx;

/*** TIM local functions ***/

/* TIM21 INTERRUPT HANDLER.
 * @param:	None.
 * @return:	None.
 */
void __attribute__((optimize("-O0"))) TIM21_IRQHandler(void) {
	// Check update flag.
	if (((TIM21 -> SR) & (0b1 << 0)) != 0) {
		// Update duty cycles.
		TIM2 -> CCR1 = TIM21_DIMMING_LUT[tim21_ctx.dimming_lut_idx];
		TIM2 -> CCR2 = TIM21_DIMMING_LUT[tim21_ctx.dimming_lut_idx];
		TIM2 -> CCR3 = TIM21_DIMMING_LUT[tim21_ctx.dimming_lut_idx];
		// Manage index and direction.
		if (tim21_ctx.dimming_lut_direction == 0) {
			// Increment index.
			tim21_ctx.dimming_lut_idx++;
			// Invert direction at end of table.
			if (tim21_ctx.dimming_lut_idx >= (TIM21_DIMMING_LUT_LENGTH - 1)) {
				tim21_ctx.dimming_lut_direction = 1;
			}
		}
		else {
			// Decrement index.
			tim21_ctx.dimming_lut_idx--;
			// Invert direction at the beginning of table.
			if (tim21_ctx.dimming_lut_idx == 0) {
				// Single blink done.
				TIM2_stop();
				TIM21_stop();
				tim21_ctx.dimming_lut_direction = 0;
				tim21_ctx.single_blink_done = 1;
			}
		}
		// Clear flag.
		TIM21 -> SR &= ~(0b1 << 0);
	}
}

/*** TIM functions ***/

/* INIT TIM2 FOR PWM OPERATION.
 * @param:	None.
 * @return:	None.
 */
void TIM2_init(void) {
	// Enable peripheral clock.
	RCC -> APB1ENR |= (0b1 << 0); // TIM2EN='1'.
	// Set PWM frequency.
	TIM2 -> ARR = TIM2_ARR_VALUE; // Timer input clock is SYSCLK (PSC=0 by default).
	// Configure channels 1-4 in PWM mode 1 (OCxM='110' and OCxPE='1').
	TIM2 -> CCMR1 |= (0b110 << 12) | (0b1 << 11) | (0b110 << 4) | (0b1 << 3);
	TIM2 -> CCMR2 |= (0b110 << 12) | (0b1 << 11) | (0b110 << 4) | (0b1 << 3);
	// Disable all channels by default (CCxE='0').
	TIM2 -> CCR1 = (TIM2_ARR_VALUE + 1);
	TIM2 -> CCR2 = (TIM2_ARR_VALUE + 1);
	TIM2 -> CCR3 = (TIM2_ARR_VALUE + 1);
	// Generate event to update registers.
	TIM2 -> EGR |= (0b1 << 0); // UG='1'.
}

/* SET CURRENT LED COLOR.
 * @param led_color:	New LED color.
 * @return:				None.
 */
void TIM2_set_color_mask(TIM2_channel_mask_t led_color) {
	// Reset bits.
	TIM2 -> CCER &= 0xFFFFEEEE;
	// Enable channels according to color.
	uint8_t idx = 0;
	for (idx=0 ; idx<TIM2_NUMBER_OF_CHANNELS ; idx++) {
		if ((led_color & (0b1 << idx)) != 0) {
			TIM2 -> CCER |= (0b1 << (4 * idx));
		}
	}
}

/* START PWM GENERATION.
 * @param:	None.
 * @return:	None.
 */
void TIM2_start(void) {
	// Link GPIOs to timer.
	GPIO_configure(&GPIO_LED_RED, GPIO_MODE_ALTERNATE_FUNCTION, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_LED_GREEN, GPIO_MODE_ALTERNATE_FUNCTION, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_LED_BLUE, GPIO_MODE_ALTERNATE_FUNCTION, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	// Enable counter.
	TIM2 -> CNT = 0;
	TIM2 -> CR1 |= (0b1 << 0); // CEN='1'.
}

/* STOP PWM GENERATION.
 * @param:	None.
 * @return:	None.
 */
void TIM2_stop(void) {
	// Disable all channels.
	TIM2 -> CCER &= 0xFFFFEEEE;
	// Disable and reset counter.
	TIM2 -> CR1 &= ~(0b1 << 0); // CEN='0'.
}

/* INIT TIM21 FOR LED BLINKING OPERATION.
 * @param:	None.
 * @return:	None.
 */
void TIM21_init(void) {
	// Enable peripheral clock.
	RCC -> APB2ENR |= (0b1 << 2); // TIM21EN='1'.
	// Configure period.
	TIM21 -> PSC = 1; // Timer is clocked on (MSI / 2) .
	// Generate event to update registers.
	TIM21 -> EGR |= (0b1 << 0); // UG='1'.
	// Enable interrupt.
	TIM21 -> DIER |= (0b1 << 0);
	// Set interrupt priority.
	NVIC_set_priority(NVIC_INTERRUPT_TIM21, NVIC_PRIORITY_MIN);
}

/* START TIM21 PERIPHERAL.
 * @param led_blink_period_ms:	LED blink period in ms.
 * @return:						None.
 */
void TIM21_start(uint32_t led_blink_period_ms) {
	// Reset LUT index and flag.
	tim21_ctx.dimming_lut_idx = 0;
	tim21_ctx.dimming_lut_direction = 0;
	tim21_ctx.single_blink_done = 0;
	// Set period.
	TIM21 -> CNT = 0;
	TIM21 -> ARR = (led_blink_period_ms * RCC_MSI_FREQUENCY_KHZ) / (4 * TIM21_DIMMING_LUT_LENGTH);
	// Clear flag and enable interrupt.
	TIM21 -> SR &= ~(0b1 << 0); // Clear flag (UIF='0').
	NVIC_enable_interrupt(NVIC_INTERRUPT_TIM21);
	// Enable TIM21 peripheral.
	TIM21 -> CR1 |= (0b1 << 0); // Enable TIM21 (CEN='1').
}

/* STOP TIM21 COUNTER.
 * @param:	None.
 * @return:	None.
 */
void TIM21_stop(void) {
	// Disable interrupt.
	NVIC_disable_interrupt(NVIC_INTERRUPT_TIM21);
	// Stop TIM21.
	TIM21 -> CR1 &= ~(0b1 << 0); // CEN='0'.
}

/* GET SINGLE BLINK STATUS.
 * @param:							None.
 * @return single_blink_done: '1' if the single blink is finished, '0' otherwise.
 */
uint8_t TIM21_is_single_blink_done(void) {
	return (tim21_ctx.single_blink_done);
}

#endif /* LVRM or DDRM or RRM */
