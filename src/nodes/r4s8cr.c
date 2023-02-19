/*
 * r4s8cr.c
 *
 *  Created on: Feb 5, 2023
 *      Author: ludo
 */

#include "r4s8cr.h"

#include "dinfox.h"
#include "lpuart.h"
#include "mode.h"
#include "node.h"

#ifdef AM

/*** R4S8CR local macros ***/

#define R4S8CR_BAUD_RATE					9600

#define R4S8CR_BUFFER_SIZE_BYTES			64
#define R4S8CR_REPLY_BUFFER_DEPTH			16

#define R4S8CR_ADDRESS_SIZE_BYTES			1
#define R4S8CR_RELAY_ADDRESS_SIZE_BYTES		1
#define R4S8CR_COMMAND_SIZE_BYTES			1

#define R4S8CR_COMMAND_READ					0xA0
#define R4S8CR_COMMAND_OFF					0x00
#define R4S8CR_COMMAND_ON					0x01

#define R4S8CR_REPLY_PARSING_DELAY_MS		10

#define R4S8CR_REPLY_SIZE_BYTES				(R4S8CR_ADDRESS_SIZE_BYTES + R4S8CR_RELAY_ADDRESS_SIZE_BYTES + R4S8CR_REGISTER_LAST)

#define R4S8CR_SIGFOX_PAYLOAD_DATA_SIZE		1

static const char_t* R4S8CR_STRING_DATA_NAME[R4S8CR_STRING_DATA_INDEX_LAST] = {
	"RELAY 1 =",
	"RELAY 2 =",
	"RELAY 3 =",
	"RELAY 4 =",
	"RELAY 5 =",
	"RELAY 6 =",
	"RELAY 7 =",
	"RELAY 8 ="
};
static const int32_t R4S8CR_ERROR_VALUE[R4S8CR_REGISTER_LAST] = {
	NODE_ERROR_VALUE_RELAY_STATE,
	NODE_ERROR_VALUE_RELAY_STATE,
	NODE_ERROR_VALUE_RELAY_STATE,
	NODE_ERROR_VALUE_RELAY_STATE,
	NODE_ERROR_VALUE_RELAY_STATE,
	NODE_ERROR_VALUE_RELAY_STATE,
	NODE_ERROR_VALUE_RELAY_STATE,
	NODE_ERROR_VALUE_RELAY_STATE
};

/*** R4S8CR local structures ***/

typedef union {
	uint8_t frame[R4S8CR_SIGFOX_PAYLOAD_DATA_SIZE];
	struct {
		unsigned relay_1 : 1;
		unsigned relay_2 : 1;
		unsigned relay_3 : 1;
		unsigned relay_4 : 1;
		unsigned relay_5 : 1;
		unsigned relay_6 : 1;
		unsigned relay_7 : 1;
		unsigned relay_8 : 1;
	} __attribute__((packed));
} R4S8CR_sigfox_payload_data_t;

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

/* CONFIGURE PHYSICAL INTERFACE.
 * @param:	None.
 * @return:	None.
 */
static NODE_status_t _R4S8CR_configure_phy(void) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	LPUART_status_t lpuart1_status = LPUART_SUCCESS;
	LPUART_config_t lpuart_config;
	// Configure physical interface.
	lpuart_config.baud_rate = R4S8CR_BAUD_RATE;
	lpuart_config.rx_mode = LPUART_RX_MODE_DIRECT;
	lpuart_config.rx_callback = &R4S8CR_fill_rx_buffer;
	lpuart1_status = LPUART1_configure(&lpuart_config);
	LPUART1_status_check(NODE_ERROR_BASE_LPUART);
errors:
	return status;
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
	uint8_t relay_box_id = 0;
	// Check parameters.
	if ((read_params == NULL) || (read_data == NULL) || (read_status == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	if ((read_params -> format) != STRING_FORMAT_BOOLEAN) {
		status = NODE_ERROR_REGISTER_FORMAT;
		goto errors;
	}
	if ((read_params -> type) != NODE_REPLY_TYPE_VALUE) {
		status = NODE_ERROR_READ_TYPE;
		goto errors;
	}
	if ((read_params -> register_address) >= R4S8CR_REGISTER_LAST) {
		status = NODE_ERROR_REGISTER_ADDRESS;
		goto errors;
	}
	if (((read_params -> node_address) < DINFOX_NODE_ADDRESS_R4S8CR_START) || ((read_params -> node_address) >= (DINFOX_NODE_ADDRESS_R4S8CR_START + DINFOX_NODE_ADDRESS_RANGE_R4S8CR))) {
		status = NODE_ERROR_NODE_ADDRESS;
		goto errors;
	}
	// Convert node address to ID.
	relay_box_id = ((read_params -> node_address) - DINFOX_NODE_ADDRESS_R4S8CR_START + 1) & 0x0F;
	// Flush buffers and status.
	_R4S8CR_flush_buffers();
	(read_status -> all) = 0;
	// Build command.
	r4s8cr_ctx.command[0] = R4S8CR_NODE_ADDRESS;
	r4s8cr_ctx.command[1] = (R4S8CR_COMMAND_READ | relay_box_id);
	r4s8cr_ctx.command[2] = 0x00;
	r4s8cr_ctx.command_size = (R4S8CR_ADDRESS_SIZE_BYTES + R4S8CR_RELAY_ADDRESS_SIZE_BYTES + R4S8CR_COMMAND_SIZE_BYTES);
	// Configure physical interface.
	status = _R4S8CR_configure_phy();
	if (status != NODE_SUCCESS) goto errors;
	LPUART1_disable_rx();
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
			(read_data -> value) = r4s8cr_ctx.reply[(read_params -> register_address) + R4S8CR_REPLY_SIZE_BYTES - R4S8CR_REGISTER_LAST];
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
	uint8_t relay_box_id = 0;
	uint8_t relay_id = 0;
	// Check parameters.
	if ((write_params == NULL) || (write_status == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	if ((write_params -> format) != STRING_FORMAT_BOOLEAN) {
		status = NODE_ERROR_REGISTER_FORMAT;
		goto errors;
	}
	if ((write_params -> register_address) >= R4S8CR_REGISTER_LAST) {
		status = NODE_ERROR_REGISTER_ADDRESS;
		goto errors;
	}
	if (((write_params -> node_address) < DINFOX_NODE_ADDRESS_R4S8CR_START) || ((write_params -> node_address) >= (DINFOX_NODE_ADDRESS_R4S8CR_START + DINFOX_NODE_ADDRESS_RANGE_R4S8CR))) {
		status = NODE_ERROR_NODE_ADDRESS;
		goto errors;
	}
	// Convert node address to ID.
	relay_box_id = ((write_params -> node_address) - DINFOX_NODE_ADDRESS_R4S8CR_START + 1) & 0x0F;
	relay_id = ((relay_box_id - 1) * 8) + (write_params -> register_address) + 1;
	// No reply after write operation.
	(write_status -> all) = 0;
	// Flush buffers.
	_R4S8CR_flush_buffers();
	// Build command.
	r4s8cr_ctx.command[0] = R4S8CR_NODE_ADDRESS;
	r4s8cr_ctx.command[1] = relay_id;
	r4s8cr_ctx.command[2] = ((write_params -> value) == 0) ? R4S8CR_COMMAND_OFF : R4S8CR_COMMAND_ON;
	r4s8cr_ctx.command_size = (R4S8CR_ADDRESS_SIZE_BYTES + R4S8CR_RELAY_ADDRESS_SIZE_BYTES + R4S8CR_COMMAND_SIZE_BYTES);
	// Configure physical interface.
	status = _R4S8CR_configure_phy();
	if (status != NODE_SUCCESS) goto errors;
	LPUART1_disable_rx();
	// Send command.
	lpuart1_status = LPUART1_send(r4s8cr_ctx.command, r4s8cr_ctx.command_size);
	LPUART1_status_check(NODE_ERROR_BASE_LPUART);
	// Enable reception.
	LPUART1_enable_rx();
errors:
	return status;
}

/* SCAN R4S8CR NODES ON BUS.
 * @param nodes_list:		Node list to fill.
 * @param nodes_list_size:	Maximum size of the list.
 * @param nodes_count:		Pointer to byte that will contain the number of LBUS nodes detected.
 * @return status:			Function execution status.
 */
NODE_status_t R4S8CR_scan(NODE_t* nodes_list, uint8_t nodes_list_size, uint8_t* nodes_count) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	NODE_read_parameters_t read_params;
	NODE_read_data_t read_data;
	NODE_access_status_t read_status;
	NODE_address_t node_address = 0;
	uint8_t node_list_idx = 0;
	// Check parameters.
	if ((nodes_list == NULL) || (nodes_count == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Reset count.
	(*nodes_count) = 0;
	// Build read input common parameters.
	read_params.format = STRING_FORMAT_BOOLEAN;
	read_params.timeout_ms = R4S8CR_TIMEOUT_MS;
	read_params.register_address = R4S8CR_REGISTER_RELAY_1;
	read_params.type = NODE_REPLY_TYPE_VALUE;
	// Loop on all addresses.
	for (node_address=DINFOX_NODE_ADDRESS_R4S8CR_START ; node_address<(DINFOX_NODE_ADDRESS_R4S8CR_START + DINFOX_NODE_ADDRESS_RANGE_R4S8CR) ; node_address++) {
		// Update read parameters.
		read_params.node_address = node_address;
		// Ping address.
		status = R4S8CR_read_register(&read_params, &read_data, &read_status);
		if (status != NODE_SUCCESS) goto errors;
		// Check reply status.
		if (read_status.all == 0) {
			// Node found.
			(*nodes_count)++;
			// Store address and reset board ID.
			nodes_list[node_list_idx].address = node_address;
			nodes_list[node_list_idx].board_id = DINFOX_BOARD_ID_R4S8CR;
		}
	}
errors:
	return status;
}

/* RETRIEVE SPECIFIC DATA OF R4S8CR NODE.
 * @param data_update:	Pointer to the data update structure.
 * @return status:		Function execution status.
 */
NODE_status_t R4S8CR_update_data(NODE_data_update_t* data_update) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	NODE_read_parameters_t read_params;
	NODE_read_data_t read_data;
	NODE_access_status_t read_status;
	uint8_t buffer_size = 0;
	// Check parameters.
	if (data_update == NULL) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	if (((data_update -> name_ptr) == NULL) || ((data_update -> value_ptr) == NULL) || ((data_update -> registers_value_ptr) == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Check index.
	if ((data_update -> string_data_index) >= R4S8CR_STRING_DATA_INDEX_LAST) {
		status = NODE_ERROR_STRING_DATA_INDEX;
		goto errors;
	}
	// Common read input parameters.
#ifdef AM
	read_params.node_address = (data_update -> node_address);
#endif
	read_params.register_address = (data_update -> string_data_index);
	read_params.type = NODE_REPLY_TYPE_VALUE;
	read_params.timeout_ms = R4S8CR_TIMEOUT_MS;
	read_params.format = STRING_FORMAT_BOOLEAN;
	// Read data.
	status = R4S8CR_read_register(&read_params, &read_data, &read_status);
	if (status != NODE_SUCCESS) goto errors;
	// Add data name.
	NODE_append_string_name((char_t*) R4S8CR_STRING_DATA_NAME[(data_update -> string_data_index)]);
	buffer_size = 0;
	// Add data value.
	if (read_status.all == 0) {
		// Update integer data.
		NODE_update_value((data_update -> string_data_index), read_data.value);
		NODE_append_string_value((read_data.value == 0) ? "OFF" : "ON");
	}
	else {
		NODE_flush_string_value();
		NODE_append_string_value(NODE_ERROR_STRING);
		NODE_update_value((data_update -> string_data_index), R4S8CR_ERROR_VALUE[(data_update -> string_data_index)]);
	}
errors:
	return status;
}

/* GET R4S8CR NODE SIGFOX PAYLOAD.
 * @param integer_data_value:	Pointer to the node registers value.
 * @param sigfox_payload_type:	Sigfox payload type.
 * @param sigfox_payload:		Pointer that will contain the specific sigfox payload of the node.
 * @param sigfox_payload_size:	Pointer to byte that will contain sigfox payload size.
 * @return status:				Function execution status.
 */
NODE_status_t R4S8CR_get_sigfox_payload(int32_t* integer_data_value, NODE_sigfox_payload_type_t sigfox_payload_type, uint8_t* sigfox_payload, uint8_t* sigfox_payload_size) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	R4S8CR_sigfox_payload_data_t sigfox_payload_data;
	uint8_t idx = 0;
	// Check parameters.
	if ((integer_data_value == NULL) || (sigfox_payload == NULL) || (sigfox_payload_size == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Check type.
	switch (sigfox_payload_type) {
	case NODE_SIGFOX_PAYLOAD_TYPE_MONITORING:
		// None monitoring frame.
		(*sigfox_payload_size) = 0;
		break;
	case NODE_SIGFOX_PAYLOAD_TYPE_DATA:
		// Build data payload.
		sigfox_payload_data.relay_1 = integer_data_value[R4S8CR_REGISTER_RELAY_1];
		sigfox_payload_data.relay_2 = integer_data_value[R4S8CR_REGISTER_RELAY_2];
		sigfox_payload_data.relay_3 = integer_data_value[R4S8CR_REGISTER_RELAY_3];
		sigfox_payload_data.relay_4 = integer_data_value[R4S8CR_REGISTER_RELAY_4];
		sigfox_payload_data.relay_5 = integer_data_value[R4S8CR_REGISTER_RELAY_5];
		sigfox_payload_data.relay_6 = integer_data_value[R4S8CR_REGISTER_RELAY_6];
		sigfox_payload_data.relay_7 = integer_data_value[R4S8CR_REGISTER_RELAY_7];
		sigfox_payload_data.relay_8 = integer_data_value[R4S8CR_REGISTER_RELAY_8];
		// Copy payload.
		for (idx=0 ; idx<R4S8CR_SIGFOX_PAYLOAD_DATA_SIZE ; idx++) {
			sigfox_payload[idx] = sigfox_payload_data.frame[idx];
		}
		(*sigfox_payload_size) = R4S8CR_SIGFOX_PAYLOAD_DATA_SIZE;
		break;
	default:
		status = NODE_ERROR_SIGFOX_PAYLOAD_TYPE;
		goto errors;
	}
errors:
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

#endif /* AM */
