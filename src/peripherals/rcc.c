/*
 * rcc.c
 *
 *  Created on: 18 apr. 2020
 *      Author: Ludo
 */

#include "rcc.h"

#include "nvic.h"
#include "pwr.h"
#include "rcc_reg.h"

/*** RCC local functions ***/

/* RCC INTERRUPT HANDLER.
 * @param:	None.
 * @return:	None.
 */
void RCC_IRQHandler(void) {
	// Clear all flags.
	RCC -> CICR |= (0b11 << 0);
}

/*** RCC functions ***/

/* INIT RCC MODULE.
 * @param:	None.
 * @return:	None.
 */
void RCC_init(void) {
	// Enable LSI and LSE ready interrupts.
	RCC -> CIER |= (0b11 << 0);
}

/* ENABLE INTERNAL LOW SPEED OSCILLATOR (38kHz INTERNAL RC).
 * @param:	None.
 * @return:	None.
 */
void RCC_enable_lsi(void) {
	// Enable LSI.
	RCC -> CSR |= (0b1 << 0); // LSION='1'.
	// Enable interrupt.
	NVIC_enable_interrupt(NVIC_INTERRUPT_RCC_CRS);
	// Wait for LSI to be stable.
	while (((RCC -> CSR) & (0b1 << 1)) == 0) {
		PWR_enter_sleep_mode();
	}
	NVIC_disable_interrupt(NVIC_INTERRUPT_RCC_CRS);
}

/* ENABLE EXTERNAL LOW SPEED OSCILLATOR (32.768kHz QUARTZ).
 * @param:	None.
 * @return:	None.
 */
void RCC_enable_lse(void) {
	// Enable LSE (32.768kHz crystal).
	RCC -> CSR |= (0b1 << 8); // LSEON='1'.
	// Enable interrupt.
	NVIC_enable_interrupt(NVIC_INTERRUPT_RCC_CRS);
	// Wait for LSE to be stable.
	while (((RCC -> CSR) & (0b1 << 9)) == 0) {
		PWR_enter_sleep_mode();
	}
	NVIC_disable_interrupt(NVIC_INTERRUPT_RCC_CRS);
}
