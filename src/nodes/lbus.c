/*
 * rs485.c
 *
 *  Created on: 28 oct 2022
 *      Author: Ludo
 */

#include "lbus.h"

#include "dinfox.h"
#include "gpio.h"
#include "iwdg.h"
#include "lptim.h"
#include "lpuart.h"
#include "mapping.h"
#include "mode.h"
#include "node_common.h"
#include "string.h"

/*** LBUS local macros ***/

// Addressing.
#define LBUS_ADDRESS_MASK				0x7F
#define LBUS_ADDRESS_LAST				LBUS_ADDRESS_MASK
#define LBUS_ADDRESS_SIZE_BYTES			1
// Framing.
#define LBUS_FRAME_END					STRING_CHAR_CR

#define LBUS_BUFFER_SIZE_BYTES			80
#define LBUS_REPLY_BUFFER_DEPTH			64

#define LBUS_REPLY_PARSING_DELAY_MS	10
#define LBUS_REPLY_TIMEOUT_MS			100
#define LBUS_SEQUENCE_TIMEOUT_MS		1000

#define LBUS_COMMAND_PING				"RS"
#define LBUS_COMMAND_WRITE_REGISTER		"RS$W="
#define LBUS_COMMAND_READ_REGISTER		"RS$R="
#define LBUS_COMMAND_SEPARATOR			","

#define LBUS_REPLY_OK					"OK"
#define LBUS_REPLY_ERROR				"ERROR"

/*** LBUS local structures ***/

#ifdef AM
typedef enum {
	LBUS_FRAME_FIELD_INDEX_DESTINATION_ADDRESS = 0,
	LBUS_FRAME_FIELD_INDEX_SOURCE_ADDRESS = (LBUS_FRAME_FIELD_INDEX_DESTINATION_ADDRESS + LBUS_ADDRESS_SIZE_BYTES),
	LBUS_FRAME_FIELD_INDEX_DATA = (LBUS_FRAME_FIELD_INDEX_SOURCE_ADDRESS + LBUS_ADDRESS_SIZE_BYTES)
} LBUS_frame_field_index_t;
#endif

typedef struct {
	uint32_t timeout_ms;
	STRING_format_t format; // Expected value format.
	NODE_reply_type_t type;
} LBUS_reply_input_t;

typedef struct {
	volatile char_t buffer[LBUS_BUFFER_SIZE_BYTES];
	volatile uint8_t size;
	volatile uint8_t line_end_flag;
	PARSER_context_t parser;
} LBUS_reply_buffer_t;

typedef struct {
	// Command buffer.
	char_t command[LBUS_BUFFER_SIZE_BYTES];
	uint8_t command_size;
#ifdef AM
	NODE_address_t self_address;
	NODE_address_t expected_source_address;
#endif
	// Response buffers.
	LBUS_reply_buffer_t reply[LBUS_REPLY_BUFFER_DEPTH];
	volatile uint8_t reply_write_idx;
	uint8_t reply_read_idx;
} LBUS_context_t;

/*** LBUS local global variables ***/

static LBUS_context_t lbus_ctx;

/*** LBUS local functions ***/

/* FLUSH COMMAND BUFFER.
 * @param:	None.
 * @return:	None.
 */
static void _LBUS_flush_command(void) {
	// Local variables.
	uint8_t idx = 0;
	// Flush buffer.
	for (idx=0 ; idx<LBUS_BUFFER_SIZE_BYTES ; idx++) lbus_ctx.command[idx] = STRING_CHAR_NULL;
	lbus_ctx.command_size = 0;
}

/* FLUSH LBUS REPLY BUFFER.
 * @param reply_index:	Reply index to reset.
 * @return:				None.
 */
static void _LBUS_flush_reply(uint8_t reply_index) {
	// Flush buffer.
	lbus_ctx.reply[reply_index].size = 0;
	// Reset flag.
	lbus_ctx.reply[reply_index].line_end_flag = 0;
	// Reset parser.
	lbus_ctx.reply[reply_index].parser.buffer = (char_t*) lbus_ctx.reply[reply_index].buffer;
	lbus_ctx.reply[reply_index].parser.buffer_size = 0;
	lbus_ctx.reply[reply_index].parser.separator_idx = 0;
	lbus_ctx.reply[reply_index].parser.start_idx = 0;
}

/* FLUSH ALL LBUS REPLY BUFFERS.
 * @param:	None.
 * @return:	None.
 */
static void _LBUS_flush_replies(void) {
	// Local variabless.
	uint8_t rep_idx = 0;
	// Reset replys buffers.
	for (rep_idx=0 ; rep_idx<LBUS_REPLY_BUFFER_DEPTH ; rep_idx++) {
		_LBUS_flush_reply(rep_idx);
	}
	// Reset index and count.
	lbus_ctx.reply_write_idx = 0;
	lbus_ctx.reply_read_idx = 0;
}

#ifdef AM
/* SEND THE COMMAND BUFFER OVER LBUS BUS.
 * @param command:				Command to send.
 * @param destination_address:	LBUS address of the destination board.
 * @return status:				Function execution status.
 */
static LBUS_status_t _LBUS_send(char_t* command, NODE_address_t destination_address) {
#else
/* SEND THE COMMAND BUFFER OVER LBUS BUS.
 * @param:			None.
 * @return status:	Function execution status.
 */
static LBUS_status_t _LBUS_send(char_t* command) {
#endif
	// Local variables.
	LBUS_status_t status = LBUS_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	LPUART_status_t lpuart1_status = LPUART_SUCCESS;
#ifdef AM
	if (destination_address > LBUS_ADDRESS_LAST) {
		status = LBUS_ERROR_NODE_ADDRESS;
		goto errors;
	}
#endif
	// Flush buffer.
	_LBUS_flush_command();
#ifdef AM
	// Add addressing header.
	lbus_ctx.command[LBUS_FRAME_FIELD_INDEX_DESTINATION_ADDRESS] = (destination_address | 0x80);
	lbus_ctx.command_size++;
	lbus_ctx.command[LBUS_FRAME_FIELD_INDEX_SOURCE_ADDRESS] = lbus_ctx.self_address;
	lbus_ctx.command_size++;
#endif
	// Add command.
	string_status = STRING_append_string(lbus_ctx.command, LBUS_BUFFER_SIZE_BYTES, command, &lbus_ctx.command_size);
	STRING_status_check(LBUS_ERROR_BASE_STRING);
	// Add LBUS ending character.
	lbus_ctx.command[lbus_ctx.command_size++] = LBUS_FRAME_END;
	// Reset replies.
	_LBUS_flush_replies();
#ifdef AM
	// Store slave address to authenticate next data reception.
	lbus_ctx.expected_source_address = destination_address;
#endif
	// Send command.
	LPUART1_disable_rx();
	lpuart1_status = LPUART1_send((uint8_t*) lbus_ctx.command, lbus_ctx.command_size, &LBUS_fill_rx_buffer);
	LPUART1_enable_rx();
	LPUART1_status_check(LBUS_ERROR_BASE_LPUART);
errors:
	return status;
}

/* WAIT FOR RECEIVING A VALUE.
 * @param reply_in_ptr:		Pointer to the reply input parameters.
 * @param reply_ptr:	Pointer to the reply output data.
 * @return status:			Function execution status.
 */
LBUS_status_t _LBUS_wait_reply(LBUS_reply_input_t* reply_in_ptr, NODE_reply_t* reply_ptr) {
	// Local variables.
	LBUS_status_t status = LBUS_SUCCESS;
	PARSER_status_t parser_status = PARSER_SUCCESS;
	LPTIM_status_t lptim1_status = LPTIM_SUCCESS;
	uint32_t reply_time_ms = 0;
	uint32_t sequence_time_ms = 0;
	uint8_t reply_count = 0;
	// Check parameters.
	if ((reply_in_ptr == NULL) || (reply_ptr == NULL)) {
		status = LBUS_ERROR_NULL_PARAMETER;
		goto errors;
	}
	if ((reply_in_ptr -> type) >= NODE_REPLY_TYPE_LAST) {
		status = LBUS_ERROR_REPLY_TYPE;
		goto errors;
	}
	// Reset output data.
	(reply_ptr -> value) = 0;
	(reply_ptr -> status).all = 0;
	(reply_ptr -> raw) = NULL;
	// Directly exit function with success status for none reply type.
	if ((reply_in_ptr -> type) == NODE_REPLY_TYPE_NONE) goto errors;
	// Main reception loop.
	while (1) {
		// Delay.
		lptim1_status = LPTIM1_delay_milliseconds(LBUS_REPLY_PARSING_DELAY_MS, 0);
		LPTIM1_status_check(LBUS_ERROR_BASE_LPTIM);
		reply_time_ms += LBUS_REPLY_PARSING_DELAY_MS;
		sequence_time_ms += LBUS_REPLY_PARSING_DELAY_MS;
		// Check write index.
		if (lbus_ctx.reply_write_idx != lbus_ctx.reply_read_idx) {
			// Check line end flag.
			if (lbus_ctx.reply[lbus_ctx.reply_read_idx].line_end_flag != 0) {
				// Increment parsing count.
				reply_count++;
				// Reset time and flag.
				reply_time_ms = 0;
				lbus_ctx.reply[lbus_ctx.reply_read_idx].line_end_flag = 0;
#ifdef AM
				// Check source address.
				if (lbus_ctx.reply[lbus_ctx.reply_read_idx].buffer[LBUS_FRAME_FIELD_INDEX_SOURCE_ADDRESS] != lbus_ctx.expected_source_address) {
					(reply_ptr -> status).source_address_mismatch = 1;
					continue;
				}
				// Skip source address before parsing.
				lbus_ctx.reply[lbus_ctx.reply_read_idx].parser.buffer = (char_t*) &(lbus_ctx.reply[lbus_ctx.reply_read_idx].buffer[LBUS_FRAME_FIELD_INDEX_DATA]);
				lbus_ctx.reply[lbus_ctx.reply_read_idx].parser.buffer_size = (lbus_ctx.reply[lbus_ctx.reply_read_idx].size > 0) ? (lbus_ctx.reply[lbus_ctx.reply_read_idx].size - LBUS_FRAME_FIELD_INDEX_DATA) : 0;
#else
				// Update buffer length.
				lbus_ctx.reply[lbus_ctx.reply_read_idx].parser.buffer_size = lbus_ctx.reply[lbus_ctx.reply_read_idx].size;
#endif
				// Parse reply.
				switch (reply_in_ptr -> type) {
				case NODE_REPLY_TYPE_RAW:
					// Do not parse.
					parser_status = PARSER_SUCCESS;
					break;
				case NODE_REPLY_TYPE_OK:
					// Compare to reference string.
					parser_status = PARSER_compare(&lbus_ctx.reply[lbus_ctx.reply_read_idx].parser, PARSER_MODE_COMMAND, LBUS_REPLY_OK);
					break;
				case NODE_REPLY_TYPE_VALUE:
					// Parse value.
					parser_status = PARSER_get_parameter(&lbus_ctx.reply[lbus_ctx.reply_read_idx].parser, (reply_in_ptr -> format), STRING_CHAR_NULL, &(reply_ptr -> value));
					break;
				default:
					break;
				}
				// Check status.
				if (parser_status == PARSER_SUCCESS) {
					// Update raw pointer, status and exit..
					(reply_ptr -> status).all = 0;
#ifdef AM
					(reply_ptr -> raw) = (char_t*) (&(lbus_ctx.reply[lbus_ctx.reply_read_idx].buffer[LBUS_FRAME_FIELD_INDEX_DATA]));
#else
					(reply_ptr -> raw) = (char_t*) (lbus_ctx.reply[lbus_ctx.reply_read_idx].buffer);
#endif
					break;
				}
				// Check error.
				parser_status = PARSER_compare(&lbus_ctx.reply[lbus_ctx.reply_read_idx].parser, PARSER_MODE_HEADER, LBUS_REPLY_ERROR);
				if (parser_status == PARSER_SUCCESS) {
					// Update output data.
					(reply_ptr -> status).error_received = 1;
					break;
				}
			}
			// Update read index.
			_LBUS_flush_reply(lbus_ctx.reply_read_idx);
			lbus_ctx.reply_read_idx = (lbus_ctx.reply_read_idx + 1) % LBUS_REPLY_BUFFER_DEPTH;
		}
		// Exit if timeout.
		if (reply_time_ms > (reply_in_ptr -> timeout_ms)) {
			// Set status to timeout if none reply has been received, otherwise the parser error code is returned.
			if (reply_count == 0) {
				(reply_ptr -> status).reply_timeout = 1;
			}
			else {
				(reply_ptr -> status).parser_error = 1;
			}
			break;
		}
		if (sequence_time_ms > LBUS_SEQUENCE_TIMEOUT_MS) {
			// Set status to timeout in any case.
			(reply_ptr -> status).sequence_timeout = 1;
			break;
		}
	}
errors:
	return status;
}

#ifdef AM
/* PING LBUS NODE.
 * @param node_address:	LBUS address to ping.
 * @param reply_ptr:	Pointer to the response status.
 * @return status:		Function execution status.
 */
LBUS_status_t _LBUS_ping(NODE_address_t node_address, NODE_reply_t* reply_ptr) {
#else
/* PING LBUS NODE.
 * @param reply_ptr:	Pointer to the response status.
 * @return status:		Function execution status.
 */
LBUS_status_t _LBUS_ping(NODE_reply_t* reply_ptr) {
#endif
	// Local variables.
	LBUS_status_t status = LBUS_SUCCESS;
	LBUS_reply_input_t reply_in;
	// Expect OK.
	reply_in.type = NODE_REPLY_TYPE_OK;
	reply_in.format = STRING_FORMAT_BOOLEAN;
	reply_in.timeout_ms = LBUS_REPLY_TIMEOUT_MS;
	// Send ping command.
#ifdef AM
	status = _LBUS_send(LBUS_COMMAND_PING, node_address);
#else
	status = _LBUS_send(LBUS_COMMAND_PING);
#endif
	if (status != LBUS_SUCCESS) goto errors;
	// Wait reply.
	status = _LBUS_wait_reply(&reply_in, reply_ptr);
	if (status != LBUS_SUCCESS) goto errors;
errors:
	return status;
}

/*** LBUS functions ***/

/* INIT LBUS INTERFACE.
 * @param self_address:	Address of this board.
 * @return:				None.
 */
#ifdef AM
void LBUS_init(NODE_address_t self_address) {
#else
/* INIT LBUS INTERFACE.
 * @param:	None.
 * @return:	None.
 */
void LBUS_init(void) {
#endif
	// Reset buffers.
	_LBUS_flush_command();
	_LBUS_flush_replies();
#ifdef AM
	// Store self address.
	lbus_ctx.self_address = self_address;
#endif
}

/* READ LBUS NODE REGISTER.
 * @param read_input_ptr:	Pointer to the read operation parameters.
 * @param reply_ptr:		Pointer to the response status.
 * @return status:			Function execution status.
 */
LBUS_status_t LBUS_read_register(NODE_read_parameters_t* read_input_ptr, NODE_reply_t* reply_ptr) {
	// Local variables.
	LBUS_status_t status = LBUS_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	LBUS_reply_input_t reply_input;
	char_t command[LBUS_BUFFER_SIZE_BYTES] = {STRING_CHAR_NULL};
	uint8_t command_size = 0;
	// Copy reply inpuy parameters.
	reply_input.type = (read_input_ptr -> type);
	reply_input.format = (read_input_ptr -> format);
	reply_input.timeout_ms = (read_input_ptr -> timeout_ms);
	// Flush command buffer.
	_LBUS_flush_command();
	// Build read command.
	string_status = STRING_append_string(command, LBUS_BUFFER_SIZE_BYTES, LBUS_COMMAND_READ_REGISTER, &command_size);
	STRING_status_check(LBUS_ERROR_BASE_STRING);
	string_status = STRING_append_value(command, LBUS_BUFFER_SIZE_BYTES, (read_input_ptr -> register_address), STRING_FORMAT_HEXADECIMAL, 0, &command_size);
	STRING_status_check(LBUS_ERROR_BASE_STRING);
	// Send command.
#ifdef AM
	status = _LBUS_send(command, read_input_ptr -> node_address);
#else
	status = _LBUS_send(command);
#endif
	if (status != LBUS_SUCCESS) goto errors;
	// Wait reply.
	status = _LBUS_wait_reply(&reply_input, reply_ptr);
errors:
	return status;
}

/* WRITE LBUS NODE REGISTER.
 * @param write_input_ptr:	Pointer to the write operation parameters.
 * @param reply_ptr:		Pointer to the response status.
 * @return status:			Function execution status.
 */
LBUS_status_t LBUS_write_register(NODE_write_parameters_t* write_input_ptr, NODE_reply_t* reply_ptr) {
	// Local variables.
	LBUS_status_t status = LBUS_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	LBUS_reply_input_t reply_input;
	char_t command[LBUS_BUFFER_SIZE_BYTES] = {STRING_CHAR_NULL};
	uint8_t command_size = 0;
	// Copy reply inpuy parameters.
	reply_input.type = NODE_REPLY_TYPE_OK;
	reply_input.format = (write_input_ptr -> format);
	reply_input.timeout_ms = (write_input_ptr -> timeout_ms);
	// Build read command.
	string_status = STRING_append_string(command, LBUS_BUFFER_SIZE_BYTES, LBUS_COMMAND_WRITE_REGISTER, &command_size);
	STRING_status_check(LBUS_ERROR_BASE_STRING);
	string_status = STRING_append_value(command, LBUS_BUFFER_SIZE_BYTES, (write_input_ptr -> register_address), STRING_FORMAT_HEXADECIMAL, 0, &command_size);
	STRING_status_check(LBUS_ERROR_BASE_STRING);
	string_status = STRING_append_string(command, LBUS_BUFFER_SIZE_BYTES, LBUS_COMMAND_SEPARATOR, &command_size);
	STRING_status_check(LBUS_ERROR_BASE_STRING);
	string_status = STRING_append_value(command, LBUS_BUFFER_SIZE_BYTES, (write_input_ptr -> value), (write_input_ptr -> format), 0, &command_size);
	STRING_status_check(LBUS_ERROR_BASE_STRING);
	// Send command.
#ifdef AM
	status = _LBUS_send(command, write_input_ptr -> node_address);
#else
	status = _LBUS_send(command);
#endif
	if (status != LBUS_SUCCESS) goto errors;
	// Wait reply.
	status = _LBUS_wait_reply(&reply_input, reply_ptr);
errors:
	return status;
}

/* SCAN ALL NODES ON BUS.
 * @param nodes_list:		Node list to fill.
 * @param nodes_list_size:	Maximum size of the list.
 * @param nodes_count:		Pointer to byte that will contain the number of LBUS nodes detected.
 * @return status:			Function execution status.
 */
LBUS_status_t LBUS_scan(NODE_t* nodes_list, uint8_t nodes_list_size, uint8_t* nodes_count) {
	// Local variables.
	LBUS_status_t status = LBUS_SUCCESS;
	NODE_read_parameters_t read_input;
	NODE_reply_t reply;
	NODE_address_t node_address = 0;
	uint8_t node_list_idx = 0;
	// Check parameters.
	if (nodes_list == NULL) {
		status = LBUS_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Reset count.
	(*nodes_count) = 0;
	// Build read input common parameters.
	read_input.format = STRING_FORMAT_HEXADECIMAL;
	read_input.timeout_ms = LBUS_REPLY_TIMEOUT_MS;
	read_input.register_address = DINFOX_REGISTER_BOARD_ID;
	read_input.type = NODE_REPLY_TYPE_VALUE;
#ifdef AM
	// Loop on all addresses.
	for (node_address=0 ; node_address<=LBUS_ADDRESS_LAST ; node_address++) {
		// Ping address.
		status = _LBUS_ping(node_address, &reply);
		if (status != LBUS_SUCCESS) goto errors;
		// Check reply status.
		if (reply.status.all == 0) {
			// Node found (even if an error was returned after ping command).
			(*nodes_count)++;
			// Store address and reset board ID.
			nodes_list[node_list_idx].address = node_address;
			nodes_list[node_list_idx].board_id = DINFOX_BOARD_ID_ERROR;
			// Get board ID.
			read_input.node_address = node_address;
			status = LBUS_read_register(&read_input, &reply);
			if (status != LBUS_SUCCESS) goto errors;
			// Check reply status.
			if (reply.status.all == 0) {
				// Update board ID.
				nodes_list[node_list_idx].board_id = (uint8_t) reply.value;
			}
			node_list_idx++;
			// Check index.
			if (node_list_idx >= nodes_list_size) break;
		}
		IWDG_reload();
	}
#else
	// Ping address.
	status = _LBUS_ping(&reply);
	if (status != LBUS_SUCCESS) goto errors;
	// Check reply status.
	if (reply.status.all == 0) {
		// Node found (even if an error was returned after ping command).
		(*nodes_count)++;
		// Store address and reset board ID.
		nodes_list[node_list_idx].address = node_address;
		nodes_list[node_list_idx].board_id = DINFOX_BOARD_ID_ERROR;
		// Get board ID.
		status = LBUS_read_register(&read_input, &reply);
		if (status != LBUS_SUCCESS) goto errors;
		// Check reply status.
		if (reply.status.all == 0) {
			// Update board ID.
			nodes_list[node_list_idx].board_id = (uint8_t) reply.value;
		}
	}
#endif
	return LBUS_SUCCESS;
errors:
	return status;
}

/* FILL LBUS BUFFER WITH A NEW BYTE (CALLED BY LPUART INTERRUPT).
 * @param rx_byte:	Incoming byte.
 * @return:			None.
 */
void LBUS_fill_rx_buffer(uint8_t rx_byte) {
	// Read current index.
	uint8_t idx = lbus_ctx.reply[lbus_ctx.reply_write_idx].size;
	// Check ending characters.
	if (rx_byte == LBUS_FRAME_END) {
		// Set flag on current buffer.
		lbus_ctx.reply[lbus_ctx.reply_write_idx].buffer[idx] = STRING_CHAR_NULL;
		lbus_ctx.reply[lbus_ctx.reply_write_idx].line_end_flag = 1;
		// Switch buffer.
		lbus_ctx.reply_write_idx = (lbus_ctx.reply_write_idx + 1) % LBUS_REPLY_BUFFER_DEPTH;
	}
	else {
		// Store incoming byte.
		lbus_ctx.reply[lbus_ctx.reply_write_idx].buffer[idx] = rx_byte;
		// Manage index.
		idx = (idx + 1) % LBUS_BUFFER_SIZE_BYTES;
		lbus_ctx.reply[lbus_ctx.reply_write_idx].size = idx;
	}
}
