/*
 * lbus.c
 *
 *  Created on: 28 oct 2022
 *      Author: Ludo
 */

#include "lbus.h"

#include "at_bus.h"
#include "dinfox.h"
#include "lpuart.h"
#include "node.h"
#include "node_common.h"
#include "types.h"

/*** LBUS local macros ***/

// Physical interface.
#define LBUS_DESTINATION_ADDRESS_MARKER		0x80
#define LBUS_ADDRESS_SIZE_BYTES				1

/*** LBUS local structures ***/

typedef enum {
	LBUS_FRAME_FIELD_INDEX_DESTINATION_ADDRESS = 0,
	LBUS_FRAME_FIELD_INDEX_SOURCE_ADDRESS = (LBUS_FRAME_FIELD_INDEX_DESTINATION_ADDRESS + LBUS_ADDRESS_SIZE_BYTES),
	LBUS_FRAME_FIELD_INDEX_DATA = (LBUS_FRAME_FIELD_INDEX_SOURCE_ADDRESS + LBUS_ADDRESS_SIZE_BYTES)
} LBUS_frame_field_index_t;

typedef struct {
	NODE_address_t self_address;
	NODE_address_t expected_slave_address;
	uint8_t source_address_mismatch;
	uint8_t rx_byte_count;
} LBUS_context_t;

/*** LBUS local global variables ***/

static LBUS_context_t lbus_ctx;

/*** LBUS local functions ***/

/* CONFIGURE PHYSICAL INTERFACE FOR LBUS.
 * @param:	None.
 * @return:	None.
 */
NODE_status_t _LBUS_configure_phy(void) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	LPUART_status_t lpuart1_status = LPUART_SUCCESS;
	LPUART_config_t lpuart_config;
	// Configure physical interface.
	lpuart_config.baud_rate = LBUS_BAUD_RATE;
	lpuart_config.rx_mode = LPUART_RX_MODE_ADDRESSED;
	lpuart_config.rx_callback = &LBUS_fill_rx_buffer;
	lpuart1_status = LPUART1_configure(&lpuart_config);
	LPUART1_check_status(NODE_ERROR_BASE_LPUART);
errors:
	return status;
}

/*** LBUS functions ***/

/* INIT LBUS LAYER.
 * @param:	None.
 * @return:	None.
 */
void LBUS_init(void) {
	// Init context.
	lbus_ctx.self_address = DINFOX_NODE_ADDRESS_DMM;
	lbus_ctx.expected_slave_address = DINFOX_NODE_ADDRESS_BROADCAST;
	lbus_ctx.source_address_mismatch = 0;
	lbus_ctx.rx_byte_count = 0;
}

/* SEND REPLY OVER LBUS.
 * @param destination_address:	Destination address.
 * @param data:					Byte array to send.
 * @param data_size_bytes:		Number of bytes to send.
 * @return status:				Function execution status.
 */
NODE_status_t LBUS_send(NODE_address_t destination_address, uint8_t* data, uint32_t data_size_bytes) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	LPUART_status_t lpuart1_status = LPUART_SUCCESS;
	uint8_t lbus_header[LBUS_FRAME_FIELD_INDEX_DATA];
	// Check address.
	if (destination_address > LBUS_ADDRESS_LAST) {
		status = NODE_ERROR_NODE_ADDRESS;
		goto errors;
	}
	// Store destination address for next reception.
	lbus_ctx.expected_slave_address = destination_address;
	// Build address header.
	lbus_header[LBUS_FRAME_FIELD_INDEX_DESTINATION_ADDRESS] = (destination_address | LBUS_DESTINATION_ADDRESS_MARKER);
	lbus_header[LBUS_FRAME_FIELD_INDEX_SOURCE_ADDRESS] = lbus_ctx.self_address;
	// Configure physical interface.
	status = _LBUS_configure_phy();
	if (status != NODE_SUCCESS) goto errors;
	// Send header.
	lpuart1_status = LPUART1_send(lbus_header, LBUS_FRAME_FIELD_INDEX_DATA);
	LPUART1_check_status(NODE_ERROR_BASE_LPUART);
	// Send command.
	lpuart1_status = LPUART1_send(data, data_size_bytes);
	LPUART1_check_status(NODE_ERROR_BASE_LPUART);
errors:
	// Reset RX byte for next reception.
	lbus_ctx.rx_byte_count = 0;
	return status;
}

/* RESET LBUS BYTE COUNT.
 * @param:	None.
 * @return:	None.
 */
void LBUS_reset(void) {
	lbus_ctx.rx_byte_count = 0;
}

/* FILL COMMAND BUFFER WITH A NEW BYTE (CALLED BY LPUART INTERRUPT).
 * @param rx_byte:	Incoming byte.
 * @return:			None.
 */
void LBUS_fill_rx_buffer(uint8_t rx_byte) {
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
		// Transmit command to applicative layer.
		if (lbus_ctx.source_address_mismatch == 0) {
			AT_BUS_fill_rx_buffer(rx_byte);
		}
		break;
	}
	// Increment byte count.
	lbus_ctx.rx_byte_count++;
}
