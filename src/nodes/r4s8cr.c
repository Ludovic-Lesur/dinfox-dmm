/*
 * r4s8cr.c
 *
 *  Created on: Feb 5, 2023
 *      Author: ludo
 */

#include "r4s8cr.h"

#include "lpuart.h"
#include "node_common.h"
#include "node_status.h"

/*** R4S8CR local macros ***/

#define R4S8CR_BUFFER_SIZE_BYTES		64
#define R4S8CR_REPLY_BUFFER_DEPTH		16

#define R4S8CR_ADDRESS_SIZE_BYTES		1
#define R4S8CR_RELAY_ADDRESS_SIZE_BYTES	1
#define R4S8CR_COMMAND_SIZE_BYTES		1

#define R4S8CR_COMMAND_READ				0xA1
#define R4S8CR_COMMAND_OFF				0x00
#define R4S8CR_COMMAND_ON				0x01

#define R4S8CR_REPLY_PARSING_DELAY_MS	10

#define R4S8CR_NUMBER_OF_RELAYS			8

#define R4S8CR_REPLY_SIZE_BYTES			(R4S8CR_ADDRESS_SIZE_BYTES + R4S8CR_RELAY_ADDRESS_SIZE_BYTES + R4S8CR_NUMBER_OF_RELAYS)

static const char_t* R4S8CR_STRING_DATA_NAME[R4S8CR_STRING_DATA_INDEX_LAST] = {"RELAY 1 =", "RELAY 2 =", "RELAY 3 =", "RELAY 4 =", "RELAY 5 =", "RELAY 6 =", "RELAY 7 =", "RELAY 8 ="};

/*** R4S8CR local structures ***/

typedef struct {
	uint8_t command[R4S8CR_BUFFER_SIZE_BYTES];
	uint8_t command_size;
	volatile uint8_t reply[R4S8CR_BUFFER_SIZE_BYTES];
	volatile uint8_t reply_size;
} R4S8CR_context_t;

/*** R4S8CR local global variables ***/

static R4S8CR_context_t r4s8cr_ctx;

/*** LBUS local functions ***/

/* FLUSH COMMAND BUFFER.
 * @param:	None.
 * @return:	None.
 */
static void _R4S8CR_flush_buffers(void) {
	// Local variables.
	uint8_t idx = 0;
	// Flush buffers.
	for (idx=0 ; idx<R4S8CR_BUFFER_SIZE_BYTES ; idx++) {
		r4s8cr_ctx.command[idx] = 0x00;
		r4s8cr_ctx.reply[idx] = 0x00;
	}
	r4s8cr_ctx.command_size = 0;
	r4s8cr_ctx.reply_size = 0;
}

/*** R4S8CR functions ***/

/* READ R4S8CR NODE REGISTER.
 * @param read_params:	Pointer to the read operation parameters.
 * @param read_data:	Pointer to the read result.
 * @param read_status:	Pointer to the read operation status.
 * @return status:		Function execution status.
 */
NODE_status_t R4S8CR_read_register(NODE_read_parameters_t* read_params, NODE_read_data_t* read_data, NODE_access_status_t* read_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	LPUART_status_t lpuart1_status = LPUART_SUCCESS;
	LPTIM_status_t lptim1_status = LPTIM_SUCCESS;
	uint32_t reply_time_ms = 0;
	// Check parameters.
	if ((read_params == NULL) || (read_data == NULL) || (read_status == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	if ((read_params -> format) != STRING_FORMAT_BOOLEAN) {
		status = NODE_ERROR_REGISTER_FORMAT;
		goto errors;
	}
	if ((read_params -> type) != NODE_READ_TYPE_VALUE) {
		status = NODE_ERROR_READ_TYPE;
		goto errors;
	}
	// Flush buffers and status.
	_R4S8CR_flush_buffers();
	(read_status -> all) = 0;
	// Build command.
	r4s8cr_ctx.command[0] = R4S8CR_RS485_ADDRESS;
	r4s8cr_ctx.command[1] = R4S8CR_COMMAND_READ;
	r4s8cr_ctx.command[2] = 0x00;
	r4s8cr_ctx.command_size = (R4S8CR_ADDRESS_SIZE_BYTES + R4S8CR_RELAY_ADDRESS_SIZE_BYTES + R4S8CR_COMMAND_SIZE_BYTES);
	// Configure reception mode.
	LPUART1_disable_rx();
	lpuart1_status = LPUART1_set_rx_mode(LPUART_RX_MODE_DIRECT, &R4S8CR_fill_rx_buffer);
	LPUART1_status_check(NODE_ERROR_BASE_LPUART);
	// Send command.
	lpuart1_status = LPUART1_send(r4s8cr_ctx.command, r4s8cr_ctx.command_size);
	LPUART1_status_check(NODE_ERROR_BASE_LPUART);
	// Enable reception.
	LPUART1_enable_rx();
	// Wait reply.
	while (1) {
		// Delay.
		lptim1_status = LPTIM1_delay_milliseconds(R4S8CR_REPLY_PARSING_DELAY_MS, 0);
		LPTIM1_status_check(NODE_ERROR_BASE_LPTIM);
		reply_time_ms += R4S8CR_REPLY_PARSING_DELAY_MS;
		// Check number of received bytes.
		if (r4s8cr_ctx.reply_size >= R4S8CR_REPLY_SIZE_BYTES) {
			// Update value.
			(read_data -> value) = r4s8cr_ctx.reply[(read_params -> register_address) + R4S8CR_REPLY_SIZE_BYTES - R4S8CR_NUMBER_OF_RELAYS];
			break;
		}
		// Exit if timeout.
		if (reply_time_ms > (read_params -> timeout_ms)) {
			// Set status to timeout.
			(read_status -> reply_timeout) = 1;
			break;
		}
	}
errors:
	return status;
}

/* WRITE R4S8CR NODE REGISTER.
 * @param write_params:	Pointer to the write operation parameters.
 * @param write_status:	Pointer to the write operation status.
 * @return status:		Function execution status.
 */
NODE_status_t R4S8CR_write_register(NODE_write_parameters_t* write_params, NODE_access_status_t* write_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	LPUART_status_t lpuart1_status = LPUART_SUCCESS;
	// Check parameters.
	if ((write_params == NULL) || (write_status == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	if ((write_params -> format) != STRING_FORMAT_BOOLEAN) {
		status = NODE_ERROR_REGISTER_FORMAT;
		goto errors;
	}
	// No reply after write operation.
	(write_status -> all) = 0;
	// Flush buffers.
	_R4S8CR_flush_buffers();
	// Build command.
	r4s8cr_ctx.command[0] = R4S8CR_RS485_ADDRESS;
	r4s8cr_ctx.command[1] = ((write_params -> register_address) + 1);
	r4s8cr_ctx.command[2] = ((write_params -> value) == 0) ? R4S8CR_COMMAND_OFF : R4S8CR_COMMAND_ON;
	r4s8cr_ctx.command_size = (R4S8CR_ADDRESS_SIZE_BYTES + R4S8CR_RELAY_ADDRESS_SIZE_BYTES + R4S8CR_COMMAND_SIZE_BYTES);
	// Configure reception mode.
	LPUART1_disable_rx();
	lpuart1_status = LPUART1_set_rx_mode(LPUART_RX_MODE_DIRECT, &R4S8CR_fill_rx_buffer);
	LPUART1_status_check(NODE_ERROR_BASE_LPUART);
	// Send command.
	lpuart1_status = LPUART1_send(r4s8cr_ctx.command, r4s8cr_ctx.command_size);
	LPUART1_status_check(NODE_ERROR_BASE_LPUART);
	// Enable reception.
	LPUART1_enable_rx();
errors:
	return status;
}

/* RETRIEVE SPECIFIC DATA OF RS48CR NODE.
 * @param rs485_address:		RS485 address of the node to update.
 * @param string_data_index:	Node string data index.
 * @param single_data_ptr:		Pointer to the data string and value to fill.
 * @return status:				Function execution status.
 */
NODE_status_t R4S8CR_update_data(NODE_address_t rs485_address, uint8_t string_data_index, NODE_single_data_ptr_t* single_data_ptr) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	NODE_read_parameters_t read_params;
	NODE_read_data_t read_data;
	NODE_access_status_t read_status;
	uint8_t buffer_size = 0;
	// Check index.
	if (string_data_index >= R4S8CR_STRING_DATA_INDEX_LAST) {
		status = NODE_ERROR_STRING_DATA_INDEX;
		goto errors;
	}
	// Common read input parameters.
#ifdef AM
	read_params.node_address = rs485_address;
#endif
	read_params.register_address = string_data_index;
	read_params.type = NODE_READ_TYPE_VALUE;
	read_params.timeout_ms = R4S8CR_TIMEOUT_MS;
	read_params.format = STRING_FORMAT_BOOLEAN;
	// Read data.
	status = R4S8CR_read_register(&read_params, &read_data, &read_status);
	if (status != NODE_SUCCESS) goto errors;
	// Add data name.
	NODE_append_string_name((char_t*) R4S8CR_STRING_DATA_NAME[string_data_index]);
	buffer_size = 0;
	// Add data value.
	if (read_status.all == 0) {
		// Update integer data.
		NODE_update_value(read_data.value);
		NODE_append_string_value((read_data.value == 0) ? "OFF" : "ON");
	}
	else {
		NODE_append_string_value(NODE_STRING_DATA_ERROR);
	}
errors:
	return status;
}

/* GET R4S8CR NODE SIGFOX PAYLOAD.
 * @param payload_type:		Sigfox payload type.
 * @param ul_payload:		Pointer that will contain UL payload of the node.
 * @param ul_payload_size:	Pointer to byte that will contain UL payload size.
 * @return status:			Function execution status.
 */
NODE_status_t R4S8CR_get_sigfox_payload(NODE_sigfox_payload_type_t payload_type, uint8_t* ul_payload, uint8_t* ul_payload_size) {
	NODE_status_t status = NODE_SUCCESS;
	return status;
}

/* FILL R4S8CR BUFFER WITH A NEW BYTE (CALLED BY LPUART INTERRUPT).
 * @param rx_byte:	Incoming byte.
 * @return:			None.
 */
void R4S8CR_fill_rx_buffer(uint8_t rx_byte) {
	// Store incoming byte.
	r4s8cr_ctx.reply[r4s8cr_ctx.reply_size] = rx_byte;
	// Manage index.
	r4s8cr_ctx.reply_size = (r4s8cr_ctx.reply_size + 1) % R4S8CR_BUFFER_SIZE_BYTES;
}
