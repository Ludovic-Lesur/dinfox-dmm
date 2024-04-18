/*
 * lpuart.c
 *
 *  Created on: 25 oct. 2022
 *      Author: Ludo
 */

#include "lpuart.h"

#include "exti.h"
#include "gpio.h"
#include "lpuart_reg.h"
#include "mapping.h"
#include "node_common.h"
#include "nvic.h"
#include "rcc.h"
#include "rcc_reg.h"
#include "types.h"

/*** LPUART local macros ***/

#define LPUART_BAUD_RATE_DEFAULT 			1200
#define LPUART_BAUD_RATE_CLOCK_THRESHOLD	4000

#define LPUART_BRR_VALUE_MIN				0x00300
#define LPUART_BRR_VALUE_MAX				0xFFFFF

#define LPUART_TIMEOUT_COUNT				100000
//#define LPUART_USE_NRE

/*** LPUART local global variables ***/

static LPUART_rx_irq_cb_t lpuart1_rx_irq_callback = NULL;

/*** LPUART local functions ***/

/*******************************************************************/
void __attribute__((optimize("-O0"))) LPUART1_IRQHandler(void) {
	// Local variables.
	uint8_t rx_byte = 0;
	// RXNE interrupt.
	if (((LPUART1 -> ISR) & (0b1 << 5)) != 0) {
		// Read incoming byte.
		rx_byte = (LPUART1 -> RDR);
		// Transmit byte to upper layer.
		if (lpuart1_rx_irq_callback != NULL) {
			lpuart1_rx_irq_callback(rx_byte);
		}
	}
	// Overrun error interrupt.
	if (((LPUART1 -> ISR) & (0b1 << 3)) != 0) {
		// Clear ORE flag.
		LPUART1 -> ICR |= (0b1 << 3);
	}
	EXTI_clear_flag(EXTI_LINE_LPUART1);
}

/*******************************************************************/
static LPUART_status_t _LPUART1_fill_tx_buffer(uint8_t tx_byte) {
	// Local variables.
	LPUART_status_t status = LPUART_SUCCESS;
	uint32_t loop_count = 0;
	// Fill transmit register.
	LPUART1 -> TDR = tx_byte;
	// Wait for transmission to complete.
	while (((LPUART1 -> ISR) & (0b1 << 7)) == 0) {
		// Wait for TXE='1' or timeout.
		loop_count++;
		if (loop_count > LPUART_TIMEOUT_COUNT) {
			status = LPUART_ERROR_TX_TIMEOUT;
			goto errors;
		}
	}
errors:
	return status;
}

/*******************************************************************/
static LPUART_status_t _LPUART1_set_baud_rate(uint32_t baud_rate) {
	// Local variables.
	LPUART_status_t status = LPUART_SUCCESS;
	RCC_status_t rcc_status = RCC_SUCCESS;
	RCC_clock_t lpuart_clock;
	uint32_t lpuart_clock_hz = 0;
	uint64_t brr = 0;
	// Ensure peripheral is disabled.
	LPUART1 -> CR1 &= ~(0b1 << 0); // UE='0'.
	// Select LPUART clock source.
	if (baud_rate < LPUART_BAUD_RATE_CLOCK_THRESHOLD) {
		// Use LSE.
		RCC -> CCIPR |= (0b1 << 10); // LPUART1SEL='11'.
		lpuart_clock = RCC_CLOCK_LSE;
	}
	else {
		// Use HSI.
		RCC -> CCIPR &= ~(0b1 << 10); // LPUART1SEL='10'.
		lpuart_clock = RCC_CLOCK_HSI;
	}
	// Get clock source frequency.
	rcc_status = RCC_get_frequency_hz(lpuart_clock, &lpuart_clock_hz);
	RCC_exit_error(LPUART_ERROR_BASE_RCC);
	// Compute register value.
	brr = ((uint64_t) lpuart_clock_hz) << 8;
	brr /= (uint64_t) baud_rate;
	// Check value.
	if ((brr < LPUART_BRR_VALUE_MIN) || (brr > LPUART_BRR_VALUE_MAX)) {
		status = LPUART_ERROR_BAUD_RATE;
		goto errors;
	}
	LPUART1 -> BRR = (uint32_t) (brr & 0x000FFFFF); // BRR = (256*fCK)/(baud rate). See p.730 of RM0377 datasheet.
errors:
	return status;
}

/*** LPUART functions ***/

/*******************************************************************/
LPUART_status_t LPUART1_init(NODE_address_t self_address) {
	// Local variables.
	LPUART_status_t status = LPUART_SUCCESS;
	// Select LSE as clock source.
	RCC -> CCIPR |= (0b11 << 10); // LPUART1SEL='11'.
	// Enable peripheral clock.
	RCC -> APB1ENR |= (0b1 << 18); // LPUARTEN='1'.
	RCC -> APB1SMENR |= (0b1 << 18); // LPUART1SMEN='1'.
	// Configure peripheral.
	LPUART1 -> CR1 |= 0x00002822;
	LPUART1 -> CR2 |= (self_address << 24) | (0b1 << 4);
	LPUART1 -> CR3 |= 0x00805000;
	// Baud rate.
	status = _LPUART1_set_baud_rate(LPUART_BAUD_RATE_DEFAULT);
	if (status != LPUART_SUCCESS) goto errors;
	// Configure interrupt.
	EXTI_configure_line(EXTI_LINE_LPUART1, EXTI_TRIGGER_RISING_EDGE);
	// Enable transmitter.
	LPUART1 -> CR1 |= (0b1 << 3); // TE='1'.
	// Enable peripheral.
	LPUART1 -> CR1 |= (0b1 << 0); // UE='1'.
	// Configure GPIOs.
	GPIO_configure(&GPIO_LPUART1_TX, GPIO_MODE_ALTERNATE_FUNCTION, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_LPUART1_RX, GPIO_MODE_ALTERNATE_FUNCTION, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_LPUART1_DE, GPIO_MODE_ALTERNATE_FUNCTION, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE); // External pull-down resistor present.
#ifdef LPUART_USE_NRE
	// Disable receiver by default.
	GPIO_configure(&GPIO_LPUART1_NRE, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE); // External pull-down resistor present.
	GPIO_write(&GPIO_LPUART1_NRE, 1);
#else
	// Put NRE pin in high impedance since it is directly connected to the DE pin.
	GPIO_configure(&GPIO_LPUART1_NRE, GPIO_MODE_ANALOG, GPIO_TYPE_OPEN_DRAIN, GPIO_SPEED_LOW, GPIO_PULL_NONE);
#endif
errors:
	return status;
}

/*******************************************************************/
void LPUART1_de_init(void) {
	// Disable LPUART alternate function.
	GPIO_configure(&GPIO_LPUART1_TX, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_LPUART1_RX, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_LPUART1_DE, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
#ifdef LPUART_USE_NRE
	GPIO_configure(&GPIO_LPUART1_NRE, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_write(&GPIO_LPUART1_NRE, 0);
#endif
	// Disable peripheral.
	LPUART1 -> CR1 &= ~(0b1 << 0); // UE='0'.
	// Disable peripheral clock.
	RCC -> APB1ENR &= ~(0b1 << 18); // LPUARTEN='0'.
}

/*******************************************************************/
void LPUART1_enable_rx(void) {
	// Clear RXNE flag if needed.
	if (((LPUART1 -> ISR) & (0b1 << 5)) != 0) {
		LPUART1 -> RQR |= (0b1 << 3);
	}
	// Enable interrupt.
	NVIC_enable_interrupt(NVIC_INTERRUPT_LPUART1, NVIC_PRIORITY_LPUART1);
	// Enable receiver.
	LPUART1 -> CR1 |= (0b1 << 2); // RE='1'.
#ifdef LPUART_USE_NRE
	GPIO_write(&GPIO_LPUART1_NRE, 0);
#endif
	// Check mode.
	if (((LPUART1 -> CR1) & (0b1 << 13)) != 0) {
		// Mute mode request.
		LPUART1 -> RQR |= (0b1 << 2); // MMRQ='1'.
	}
}

/*******************************************************************/
void LPUART1_disable_rx(void) {
	// Disable receiver.
#ifdef LPUART_USE_NRE
	GPIO_write(&GPIO_LPUART1_NRE, 1);
#endif
	LPUART1 -> CR1 &= ~(0b1 << 2); // RE='0'.
	// Disable interrupt.
	NVIC_disable_interrupt(NVIC_INTERRUPT_LPUART1);
}

/*******************************************************************/
LPUART_status_t LPUART1_configure(LPUART_configuration_t* config) {
	// Local variables.
	LPUART_status_t status = LPUART_SUCCESS;
	// Check parameters.
	if (config == NULL) {
		status = LPUART_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Temporary disable peripheral while configuring.
	LPUART1 -> CR1 &= ~(0b1 << 0); // UE='0'.
	// Set baud rate.
	status = _LPUART1_set_baud_rate(config -> baud_rate);
	if (status != LPUART_SUCCESS) goto errors;
	// Set RX mode.
	switch (config -> rx_mode) {
	case LPUART_RX_MODE_ADDRESSED:
		// Enable mute mode, address detection and wake up on address match.
		LPUART1 -> CR1 |= 0x00002800; // MME='1' and WAKE='1'.
		LPUART1 -> CR3 &= 0xFFCFFFFF; // WUS='00'.
		break;
	case LPUART_RX_MODE_DIRECT:
		// Disable mute mode, address detection and wake-up on RXNE.
		LPUART1 -> CR1 &= 0xFFFFD7FF; // MME='0' and WAKE='0'.
		LPUART1 -> CR3 |= 0x00030000; // WUS='11'.
		break;
	default:
		status = LPUART_ERROR_RX_MODE;
		goto errors;
	}
	// Register callback.
	lpuart1_rx_irq_callback = (config -> rx_callback);
errors:
	// Enable peripheral.
	LPUART1 -> CR1 |= (0b1 << 0); // UE='1'.
	return status;
}

/*******************************************************************/
LPUART_status_t LPUART1_write(uint8_t* data, uint32_t data_size_bytes) {
	// Local variables.
	LPUART_status_t status = LPUART_SUCCESS;
	uint32_t idx = 0;
	uint32_t loop_count = 0;
	// Check parameter.
	if (data == NULL) {
		status = LPUART_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Fill TX buffer with new bytes.
	for (idx=0 ; idx<data_size_bytes ; idx++) {
		status = _LPUART1_fill_tx_buffer(data[idx]);
		if (status != LPUART_SUCCESS) goto errors;
	}
	// Wait for TC flag.
	while (((LPUART1 -> ISR) & (0b1 << 6)) == 0) {
		// Exit if timeout.
		loop_count++;
		if (loop_count > LPUART_TIMEOUT_COUNT) {
			status = LPUART_ERROR_TC_TIMEOUT;
			goto errors;
		}
	}
errors:
	return status;
}
