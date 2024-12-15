/*
 * lbus.c
 *
 *  Created on: 28 oct 2022
 *      Author: Ludo
 */

#include "lbus.h"

#include "dinfox.h"
#include "lpuart.h"
#include "node_common.h"
#include "types.h"

/*** LBUS local macros ***/

#define LBUS_BAUD_RATE						1200
#define LBUS_DESTINATION_ADDRESS_MARKER		0x80
#define LBUS_ADDRESS_SIZE_BYTES				1

/*** LBUS local structures ***/

/*******************************************************************/
typedef enum {
	LBUS_FRAME_FIELD_INDEX_DESTINATION_ADDRESS = 0,
	LBUS_FRAME_FIELD_INDEX_SOURCE_ADDRESS = (LBUS_FRAME_FIELD_INDEX_DESTINATION_ADDRESS + LBUS_ADDRESS_SIZE_BYTES),
	LBUS_FRAME_FIELD_INDEX_DATA = (LBUS_FRAME_FIELD_INDEX_SOURCE_ADDRESS + LBUS_ADDRESS_SIZE_BYTES)
} LBUS_frame_field_index_t;

/*******************************************************************/
typedef struct {
	NODE_address_t expected_slave_address;
	uint8_t source_address_mismatch;
	uint8_t rx_byte_count;
	LBUS_rx_irq_cb rx_irq_callback;
} LBUS_context_t;

/*** LBUS local global variables ***/

static LBUS_context_t lbus_ctx;

/*** LBUS local functions ***/

/*******************************************************************/
static void _LBUS_fill_rx_buffer(uint8_t rx_byte) {
	// Check field index.
	switch (lbus_ctx.rx_byte_count) {
	case LBUS_FRAME_FIELD_INDEX_DESTINATION_ADDRESS:
		// Nothing to do.
		break;
	case LBUS_FRAME_FIELD_INDEX_SOURCE_ADDRESS:
		// Check source address.
		lbus_ctx.source_address_mismatch = ((rx_byte & LBUS_ADDRESS_MASK) == lbus_ctx.expected_slave_address) ? 0 : 1;
		break;
	default:
		// Transmit byte to upper layer.
		if ((lbus_ctx.rx_irq_callback != NULL) && (lbus_ctx.source_address_mismatch == 0)) {
			lbus_ctx.rx_irq_callback(rx_byte);
		}
		break;
	}
	// Increment byte count.
	lbus_ctx.rx_byte_count++;
}

/*******************************************************************/
static LBUS_status_t _LBUS_configure_phy(void) {
	// Local variables.
	LBUS_status_t status = LBUS_SUCCESS;
	LPUART_status_t lpuart1_status = LPUART_SUCCESS;
	LPUART_configuration_t lpuart_config;
	// Configure physical interface.
	lpuart_config.baud_rate = LBUS_BAUD_RATE;
	lpuart_config.rx_mode = LPUART_RX_MODE_ADDRESSED;
	lpuart_config.rx_callback = &_LBUS_fill_rx_buffer;
	lpuart1_status = LPUART1_configure(&lpuart_config);
	LPUART1_exit_error(LBUS_ERROR_BASE_LPUART1);
errors:
	return status;
}

/*** LBUS functions ***/

/*******************************************************************/
void LBUS_init(LBUS_rx_irq_cb irq_callback) {
	// Init context.
	lbus_ctx.expected_slave_address = DINFOX_NODE_ADDRESS_BROADCAST;
	lbus_ctx.source_address_mismatch = 0;
	lbus_ctx.rx_byte_count = 0;
	lbus_ctx.rx_irq_callback = irq_callback;
}

/*******************************************************************/
LBUS_status_t LBUS_send(NODE_address_t destination_address, uint8_t* data, uint32_t data_size_bytes) {
	// Local variables.
	LBUS_status_t status = LBUS_SUCCESS;
	LPUART_status_t lpuart1_status = LPUART_SUCCESS;
	uint8_t lbus_header[LBUS_FRAME_FIELD_INDEX_DATA];
	// Check address.
	if (destination_address > LBUS_ADDRESS_LAST) {
		status = LBUS_ERROR_ADDRESS;
		goto errors;
	}
	// Store destination address for next reception.
	lbus_ctx.expected_slave_address = destination_address;
	// Build address header.
	lbus_header[LBUS_FRAME_FIELD_INDEX_DESTINATION_ADDRESS] = (destination_address | LBUS_DESTINATION_ADDRESS_MARKER);
	lbus_header[LBUS_FRAME_FIELD_INDEX_SOURCE_ADDRESS] = DINFOX_NODE_ADDRESS_DMM;
	// Configure physical interface.
	status = _LBUS_configure_phy();
	if (status != LBUS_SUCCESS) goto errors;
	// Send header.
	lpuart1_status = LPUART1_write(lbus_header, LBUS_FRAME_FIELD_INDEX_DATA);
	LPUART1_exit_error(LBUS_ERROR_BASE_LPUART1);
	// Send command.
	lpuart1_status = LPUART1_write(data, data_size_bytes);
	LPUART1_exit_error(LBUS_ERROR_BASE_LPUART1);
errors:
	// Reset RX byte for next reception.
	lbus_ctx.rx_byte_count = 0;
	return status;
}

/*******************************************************************/
void LBUS_reset(void) {
	lbus_ctx.rx_byte_count = 0;
}
