/*
 * lpuart.c
 *
 *  Created on: 25 oct. 2022
 *      Author: Ludo
 */

#include "lpuart.h"

#include "dinfox.h"
#include "exti.h"
#include "gpio.h"
#include "lptim.h"
#include "lpuart_reg.h"
#include "mapping.h"
#include "nvic.h"
#include "rcc.h"
#include "rcc_reg.h"

/*** LPUART local macros ***/

#define LPUART_BAUD_RATE_DEFAULT 	1200
#define LPUART_BRR_VALUE_MIN_LSE	0x0300

#define LPUART_TIMEOUT_COUNT		100000
//#define LPUART_USE_NRE

/*** LPUART local global variables ***/

static LPUART_rx_callback_t LPUART1_rx_callback;

/*** LPUART local functions ***/

/* LPUART1 INTERRUPT HANDLER.
 * @param:	None.
 * @return:	None.
 */
void LPUART1_IRQHandler(void) {
	// Local variables.
	uint8_t rx_byte = 0;
	// RXNE interrupt.
	if (((LPUART1 -> ISR) & (0b1 << 5)) != 0) {
		// Read incoming byte.
		rx_byte = (LPUART1 -> RDR);
		if (LPUART1_rx_callback != NULL) {
			LPUART1_rx_callback(rx_byte);
		}
		// Clear RXNE flag.
		LPUART1 -> RQR |= (0b1 << 3);
	}
	// Overrun error interrupt.
	if (((LPUART1 -> ISR) & (0b1 << 3)) != 0) {
		// Clear ORE flag.
		LPUART1 -> ICR |= (0b1 << 3);
	}
	EXTI_clear_flag(EXTI_LINE_LPUART1);
}

/* FILL LPUART1 TX BUFFER WITH A NEW BYTE.
 * @param tx_byte:	Byte to append.
 * @return status:	Function execution status.
 */
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

/* FILL LPUART1 TX BUFFER WITH A NEW BYTE.
 * @param tx_byte:	Byte to append.
 * @return status:	Function execution status.
 */
static LPUART_status_t _LPUART1_set_baud_rate(uint32_t baud_rate) {
	// Local variables.
	LPUART_status_t status = LPUART_SUCCESS;
	uint32_t brr = 0;
	// Compute register value.
	brr = (RCC_LSE_FREQUENCY_HZ * 256);
	brr /= baud_rate;
	// Check value.
	if (brr < LPUART_BRR_VALUE_MIN_LSE) {
		status = LPUART_ERROR_BAUD_RATE;
		goto errors;
	}
	LPUART1 -> BRR = (brr & 0x000FFFFF); // BRR = (256*fCK)/(baud rate). See p.730 of RM0377 datasheet.
errors:
	return status;
}

/*** LPUART functions ***/

/* CONFIGURE LPUART1.
 * @param:	None.
 * @return:	None.
 */
void LPUART1_init(void) {
	// Select LSE as clock source.
	RCC -> CCIPR |= (0b11 << 10); // LPUART1SEL='11'.
	// Enable peripheral clock.
	RCC -> APB1ENR |= (0b1 << 18); // LPUARTEN='1'.
	// Configure power enable pin.
	GPIO_configure(&GPIO_TRX_POWER_ENABLE, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	LPUART1_power_off();
#ifdef LPUART_USE_NRE
	// Disable receiver by default.
	GPIO_configure(&GPIO_LPUART1_NRE, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE); // External pull-down resistor present.
	GPIO_write(&GPIO_LPUART1_NRE, 1);
#else
	// Put NRE pin in high impedance since it is directly connected to the DE pin.
	GPIO_configure(&GPIO_LPUART1_NRE, GPIO_MODE_ANALOG, GPIO_TYPE_OPEN_DRAIN, GPIO_SPEED_LOW, GPIO_PULL_NONE);
#endif
	// Configure peripheral in addressed mode by default.
	LPUART1 -> CR1 |= 0x00002822;
	LPUART1 -> CR2 |= (DINFOX_NODE_ADDRESS_DMM << 24) | (0b1 << 4);
	LPUART1 -> CR3 |= 0x00805000;
	// Baud rate.
	_LPUART1_set_baud_rate(LPUART_BAUD_RATE_DEFAULT);
	// Configure interrupt.
	NVIC_set_priority(NVIC_INTERRUPT_LPUART1, 0);
	EXTI_configure_line(EXTI_LINE_LPUART1, EXTI_TRIGGER_RISING_EDGE);
	// Enable transmitter.
	LPUART1 -> CR1 |= (0b1 << 3); // TE='1'.
	// Enable peripheral.
	LPUART1 -> CR1 |= (0b1 << 0); // UE='1'.
}

/* TURN BUS INTERFACE ON.
 * @param:			None.
 * @return status:	Function execution status.
 */
LPUART_status_t LPUART1_power_on(void) {
	// Local variables.
	LPUART_status_t status = LPUART_SUCCESS;
	LPTIM_status_t lptim1_status = LPTIM_SUCCESS;
	// Turn transceiver on.
	GPIO_write(&GPIO_TRX_POWER_ENABLE, 1);
	// Connect pins to LPUART peripheral.
	GPIO_configure(&GPIO_LPUART1_TX, GPIO_MODE_ALTERNATE_FUNCTION, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_LPUART1_RX, GPIO_MODE_ALTERNATE_FUNCTION, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_LPUART1_DE, GPIO_MODE_ALTERNATE_FUNCTION, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE); // External pull-down resistor present.
	// Power on delay.
	lptim1_status = LPTIM1_delay_milliseconds(100, LPTIM_DELAY_MODE_STOP);
	LPTIM1_status_check(LPUART_ERROR_BASE_LPTIM);
errors:
	return status;
}

/* TURN BUS INTERFACE OFF.
 * @param:	None.
 * @return:	None.
 */
void LPUART1_power_off(void) {
	// Set pins as output low.
	GPIO_configure(&GPIO_LPUART1_TX, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_LPUART1_RX, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_LPUART1_DE, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE); // External pull-down resistor present.
	// Turn transceiver on.
	GPIO_write(&GPIO_TRX_POWER_ENABLE, 0);
}

/* ENABLE LPUART RX OPERATION.
 * @param:	None.
 * @return:	None.
 */
void LPUART1_enable_rx(void) {
#ifdef AM
	// Mute mode request.
	LPUART1 -> RQR |= (0b1 << 2); // MMRQ='1'.
#endif
	// Clear flag and enable interrupt.
	LPUART1 -> RQR |= (0b1 << 3);
	NVIC_enable_interrupt(NVIC_INTERRUPT_LPUART1);
	// Enable receiver.
	LPUART1 -> CR1 |= (0b1 << 2); // RE='1'.
#ifdef LPUART_USE_NRE
	GPIO_write(&GPIO_LPUART1_NRE, 0);
#endif
}

/* DISABLE LPUART RX OPERATION.
 * @param:	None.
 * @return:	None.
 */
void LPUART1_disable_rx(void) {
	// Disable receiver.
#ifdef LPUART_USE_NRE
	GPIO_write(&GPIO_LPUART1_NRE, 1);
#endif
	LPUART1 -> CR1 &= ~(0b1 << 2); // RE='0'.
	// Disable interrupt.
	NVIC_disable_interrupt(NVIC_INTERRUPT_LPUART1);
}

/* SET LPUART RECEPTION MODE.
 * @param config:		Pointer to the LPUART configuration structure.
 * @param rx_callback:	RX callback to call on data reception.
 * @return status:		Function execution status.
 */
LPUART_status_t LPUART1_configure(LPUART_config_t* config) {
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
	// Store callback (even if NULL).
	LPUART1_rx_callback = (config -> rx_callback);
errors:
	// Enable peripheral.
	LPUART1 -> CR1 |= (0b1 << 0); // UE='1'.
	return status;
}

/* SEND DATA OVER LPUART.
 * @param data:				Data buffer to send.
 * @param data_size_bytes:	Number of bytes to send.
 * @return status:			Function execution status.
 */
LPUART_status_t LPUART1_send(uint8_t* data, uint8_t data_size_bytes) {
	// Local variables.
	LPUART_status_t status = LPUART_SUCCESS;
	uint8_t idx = 0;
	uint32_t loop_count = 0;
	// Check parameters.
	if (data == NULL) {
		status = LPUART_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Bytes loop.
	for (idx=0 ; idx<data_size_bytes ; idx++) {
		// Fill buffer.
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
