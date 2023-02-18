/*
 * at.c
 *
 *  Created on: Feb 18, 2023
 *      Author: ludo
 */

#include "at.h"

#include "dinfox.h"
#include "gpio.h"
#include "iwdg.h"
#include "lbus.h"
#include "lptim.h"
#include "lpuart.h"
#include "mapping.h"
#include "parser.h"
#include "mode.h"
#include "node.h"
#include "string.h"

/*** AT local macros ***/

#define AT_FRAME_END				STRING_CHAR_CR

#define AT_BUFFER_SIZE_BYTES		64
#define AT_REPLY_BUFFER_DEPTH		16

#define AT_REPLY_PARSING_DELAY_MS	10
#define AT_SEQUENCE_TIMEOUT_MS		120000

#define AT_COMMAND_PING				"AT"
#define AT_COMMAND_WRITE_REGISTER	"AT$W="
#define AT_COMMAND_READ_REGISTER	"AT$R="
#define AT_COMMAND_SEPARATOR		","

#define AT_REPLY_OK					"OK"
#define AT_REPLY_ERROR				"ERROR"

/*** AT local structures ***/

typedef struct {
	volatile char_t buffer[AT_BUFFER_SIZE_BYTES];
	volatile uint8_t size;
	volatile uint8_t line_end_flag;
	PARSER_context_t parser;
} AT_reply_buffer_t;

typedef struct {
	// Command buffer.
	char_t command[AT_BUFFER_SIZE_BYTES];
	uint8_t command_size;
	// Response buffers.
	AT_reply_buffer_t reply[AT_REPLY_BUFFER_DEPTH];
	volatile uint8_t reply_write_idx;
	uint8_t reply_read_idx;
} AT_context_t;

/*** AT local global variables ***/

static AT_context_t at_ctx;

/*** AT local functions ***/

/* FLUSH COMMAND BUFFER.
 * @param:	None.
 * @return:	None.
 */
static void _AT_flush_command(void) {
	// Local variables.
	uint8_t idx = 0;
	// Flush buffer.
	for (idx=0 ; idx<AT_BUFFER_SIZE_BYTES ; idx++) at_ctx.command[idx] = STRING_CHAR_NULL;
	at_ctx.command_size = 0;
}

/* FLUSH AT REPLY BUFFER.
 * @param reply_index:	Reply index to reset.
 * @return:				None.
 */
static void _AT_flush_reply(uint8_t reply_index) {
	// Flush buffer.
	at_ctx.reply[reply_index].size = 0;
	// Reset flag.
	at_ctx.reply[reply_index].line_end_flag = 0;
	// Reset parser.
	at_ctx.reply[reply_index].parser.buffer = (char_t*) at_ctx.reply[reply_index].buffer;
	at_ctx.reply[reply_index].parser.buffer_size = 0;
	at_ctx.reply[reply_index].parser.separator_idx = 0;
	at_ctx.reply[reply_index].parser.start_idx = 0;
}

/* FLUSH ALL AT REPLY BUFFERS.
 * @param:	None.
 * @return:	None.
 */
static void _AT_flush_replies(void) {
	// Local variabless.
	uint8_t rep_idx = 0;
	// Reset replys buffers.
	for (rep_idx=0 ; rep_idx<AT_REPLY_BUFFER_DEPTH ; rep_idx++) {
		_AT_flush_reply(rep_idx);
	}
	// Reset index and count.
	at_ctx.reply_write_idx = 0;
	at_ctx.reply_read_idx = 0;
}

/* WAIT FOR RECEIVING A VALUE.
 * @param reply_params:	Pointer to the reply parameters.
 * @param read_data:	Pointer to the reply data.
 * @param reply_status:	Pointer to the reply waiting operation status.
 * @return status:		Function execution status.
 */
NODE_status_t _AT_wait_reply(NODE_reply_parameters_t* reply_params, NODE_read_data_t* read_data, NODE_access_status_t* reply_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	PARSER_status_t parser_status = PARSER_SUCCESS;
	LPTIM_status_t lptim1_status = LPTIM_SUCCESS;
	uint32_t reply_time_ms = 0;
	uint32_t sequence_time_ms = 0;
	uint8_t reply_count = 0;
	// Check parameters.
	if ((reply_params == NULL) || (read_data == NULL) || (reply_status == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	if ((reply_params -> type) >= NODE_REPLY_TYPE_LAST) {
		status = NODE_ERROR_READ_TYPE;
		goto errors;
	}
	// Reset output data.
	(read_data -> value) = 0;
	(read_data -> raw) = NULL;
	(reply_status -> all) = 0;
	// Directly exit function with success status for none reply type.
	if ((reply_params -> type) == NODE_REPLY_TYPE_NONE) goto errors;
	// Main reception loop.
	while (1) {
		// Delay.
		lptim1_status = LPTIM1_delay_milliseconds(AT_REPLY_PARSING_DELAY_MS, 0);
		LPTIM1_status_check(NODE_ERROR_BASE_LPTIM);
		reply_time_ms += AT_REPLY_PARSING_DELAY_MS;
		sequence_time_ms += AT_REPLY_PARSING_DELAY_MS;
		// Check write index.
		if (at_ctx.reply_write_idx != at_ctx.reply_read_idx) {
			// Check line end flag.
			if (at_ctx.reply[at_ctx.reply_read_idx].line_end_flag != 0) {
				// Increment parsing count.
				reply_count++;
				// Reset time and flag.
				reply_time_ms = 0;
				at_ctx.reply[at_ctx.reply_read_idx].line_end_flag = 0;
				// Update buffer length.
				at_ctx.reply[at_ctx.reply_read_idx].parser.buffer_size = at_ctx.reply[at_ctx.reply_read_idx].size;
				// Parse reply.
				switch (reply_params -> type) {
				case NODE_REPLY_TYPE_RAW:
					// Do not parse.
					parser_status = PARSER_SUCCESS;
					break;
				case NODE_REPLY_TYPE_OK:
					// Compare to reference string.
					parser_status = PARSER_compare(&at_ctx.reply[at_ctx.reply_read_idx].parser, PARSER_MODE_COMMAND, AT_REPLY_OK);
					break;
				case NODE_REPLY_TYPE_VALUE:
					// Parse value.
					parser_status = PARSER_get_parameter(&at_ctx.reply[at_ctx.reply_read_idx].parser, (reply_params -> format), STRING_CHAR_NULL, &(read_data -> value));
					break;
				default:
					break;
				}
				// Check status.
				if (parser_status == PARSER_SUCCESS) {
					// Update raw pointer, status and exit..
					(reply_status -> all) = 0;
					(read_data -> raw) = (char_t*) (at_ctx.reply[at_ctx.reply_read_idx].buffer);
					break;
				}
				// Check error.
				parser_status = PARSER_compare(&at_ctx.reply[at_ctx.reply_read_idx].parser, PARSER_MODE_HEADER, AT_REPLY_ERROR);
				if (parser_status == PARSER_SUCCESS) {
					// Update output data.
					(reply_status -> error_received) = 1;
					break;
				}
			}
			// Update read index.
			_AT_flush_reply(at_ctx.reply_read_idx);
			at_ctx.reply_read_idx = (at_ctx.reply_read_idx + 1) % AT_REPLY_BUFFER_DEPTH;
		}
		// Exit if timeout.
		if (reply_time_ms > (reply_params -> timeout_ms)) {
			// Set status to timeout if none reply has been received, otherwise the parser error code is returned.
			if (reply_count == 0) {
				(reply_status -> reply_timeout) = 1;
			}
			else {
				(reply_status -> parser_error) = 1;
			}
			break;
		}
		if (sequence_time_ms > AT_SEQUENCE_TIMEOUT_MS) {
			// Set status to timeout in any case.
			(reply_status -> sequence_timeout) = 1;
			break;
		}
	}
errors:
	return status;
}

#ifdef AM
/* PING AT NODE.
 * @param node_address:	AT address to ping.
 * @param ping_status:	Pointer to the ping operation status.
 * @return status:		Function execution status.
 */
NODE_status_t _AT_ping(NODE_address_t node_address, NODE_access_status_t* ping_status) {
#else
/* PING AT NODE.
 * @param ping_status:	Pointer to the ping operation status.
 * @return status:		Function execution status.
 */
NODE_status_t _AT_ping(NODE_access_status_t* ping_status) {
#endif
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	NODE_command_parameters_t command_params;
	NODE_reply_parameters_t reply_params;
	NODE_read_data_t unused_read_data;
	char_t command[AT_BUFFER_SIZE_BYTES] = {STRING_CHAR_NULL};
	uint8_t command_size = 0;
	// Build command structure.
#ifdef AM
	command_params.node_address = node_address;
#endif
	command_params.command = (char_t*) command;
	// Build reply structure.
	reply_params.type = NODE_REPLY_TYPE_OK;
	reply_params.format = STRING_FORMAT_BOOLEAN;
	reply_params.timeout_ms = AT_DEFAULT_TIMEOUT_MS;
	// Build read command.
	string_status = STRING_append_string(command, AT_BUFFER_SIZE_BYTES, AT_COMMAND_PING, &command_size);
	STRING_status_check(NODE_ERROR_BASE_STRING);
	// Send ping command.
	status = AT_send_command(&command_params, &reply_params, &unused_read_data, ping_status);
errors:
	return status;
}

/*** AT functions ***/

NODE_status_t AT_send_command(NODE_command_parameters_t* command_params, NODE_reply_parameters_t* reply_params, NODE_read_data_t* read_data, NODE_access_status_t* command_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
#ifdef DM
	LPUART_status_t lpuart1_status = LPUART_SUCCESS;
#endif
	// Flush buffer.
	_AT_flush_command();
	// Add command.
	string_status = STRING_append_string(at_ctx.command, AT_BUFFER_SIZE_BYTES, (command_params -> command), &at_ctx.command_size);
	STRING_status_check(NODE_ERROR_BASE_STRING);
	// Add AT ending character.
	at_ctx.command[at_ctx.command_size++] = AT_FRAME_END;
	// Reset replies.
	_AT_flush_replies();
	// Send command.
	LPUART1_disable_rx();
#ifdef AM
	status = LBUS_send((command_params -> node_address), (uint8_t*) at_ctx.command, at_ctx.command_size);
	if (status != NODE_SUCCESS) goto errors;
#else
	lpuart1_status = LPUART1_send((uint8_t*) at_ctx.command, at_ctx.command_size);
	LPUART1_status_check(NODE_ERROR_BASE_LPUART);
#endif
	LPUART1_enable_rx();
	// Wait reply.
	status = _AT_wait_reply(reply_params, read_data, command_status);
	if (status != NODE_SUCCESS) goto errors;
errors:
	return status;
}

/* READ AT NODE REGISTER.
 * @param read_params:	Pointer to the read operation parameters.
 * @param read_data:	Pointer to the read result.
 * @param read_status:	Pointer to the read operation status.
 * @return status:		Function execution status.
 */
NODE_status_t AT_read_register(NODE_read_parameters_t* read_params, NODE_read_data_t* read_data, NODE_access_status_t* read_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	NODE_command_parameters_t command_params;
	NODE_reply_parameters_t reply_params;
	char_t command[AT_BUFFER_SIZE_BYTES] = {STRING_CHAR_NULL};
	uint8_t command_size = 0;
	// Build command structure.
#ifdef AM
	command_params.node_address = (read_params -> node_address);
#endif
	command_params.command = (char_t*) command;
	// Build reply structure.
	reply_params.type = (read_params -> type);
	reply_params.format = (read_params -> format);
	reply_params.timeout_ms = (read_params -> timeout_ms);
	// Build read command.
	string_status = STRING_append_string(command, AT_BUFFER_SIZE_BYTES, AT_COMMAND_READ_REGISTER, &command_size);
	STRING_status_check(NODE_ERROR_BASE_STRING);
	string_status = STRING_append_value(command, AT_BUFFER_SIZE_BYTES, (read_params -> register_address), STRING_FORMAT_HEXADECIMAL, 0, &command_size);
	STRING_status_check(NODE_ERROR_BASE_STRING);
	// Send command.
	status = AT_send_command(&command_params, &reply_params, read_data, read_status);
errors:
	return status;
}

/* WRITE AT NODE REGISTER.
 * @param write_params:	Pointer to the write operation parameters.
 * @param write_status:	Pointer to the write operation status.
 * @return status:		Function execution status.
 */
NODE_status_t AT_write_register(NODE_write_parameters_t* write_params, NODE_access_status_t* write_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	NODE_command_parameters_t command_params;
	NODE_reply_parameters_t reply_params;
	NODE_read_data_t unused_read_data;
	char_t command[AT_BUFFER_SIZE_BYTES] = {STRING_CHAR_NULL};
	uint8_t command_size = 0;
	// Build command structure.
#ifdef AM
	command_params.node_address = (write_params -> node_address);
#endif
	command_params.command = (char_t*) command;
	// Build reply structure.
	reply_params.type = NODE_REPLY_TYPE_OK;
	reply_params.format = (write_params -> format);
	reply_params.timeout_ms = (write_params -> timeout_ms);
	// Build read command.
	string_status = STRING_append_string(command, AT_BUFFER_SIZE_BYTES, AT_COMMAND_WRITE_REGISTER, &command_size);
	STRING_status_check(NODE_ERROR_BASE_STRING);
	string_status = STRING_append_value(command, AT_BUFFER_SIZE_BYTES, (write_params -> register_address), STRING_FORMAT_HEXADECIMAL, 0, &command_size);
	STRING_status_check(NODE_ERROR_BASE_STRING);
	string_status = STRING_append_string(command, AT_BUFFER_SIZE_BYTES, AT_COMMAND_SEPARATOR, &command_size);
	STRING_status_check(NODE_ERROR_BASE_STRING);
	string_status = STRING_append_value(command, AT_BUFFER_SIZE_BYTES, (write_params -> value), (write_params -> format), 0, &command_size);
	STRING_status_check(NODE_ERROR_BASE_STRING);
	// Send command.
	status = AT_send_command(&command_params, &reply_params, &unused_read_data, write_status);
errors:
	return status;
}

/* SCAN AT NODES ON BUS.
 * @param nodes_list:		Node list to fill.
 * @param nodes_list_size:	Maximum size of the list.
 * @param nodes_count:		Pointer to byte that will contain the number of AT nodes detected.
 * @return status:			Function execution status.
 */
NODE_status_t AT_scan(NODE_t* nodes_list, uint8_t nodes_list_size, uint8_t* nodes_count) {
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
	read_params.format = STRING_FORMAT_HEXADECIMAL;
	read_params.timeout_ms = AT_DEFAULT_TIMEOUT_MS;
	read_params.register_address = DINFOX_REGISTER_BOARD_ID;
	read_params.type = NODE_REPLY_TYPE_VALUE;
#ifdef AM
	// Loop on all addresses.
	for (node_address=0 ; node_address<=DINFOX_RS485_ADDRESS_LBUS_LAST ; node_address++) {
		// Ping address.
		status = _AT_ping(node_address, &read_status);
		if (status != NODE_SUCCESS) goto errors;
		// Check reply status.
		if (read_status.all == 0) {
			// Node found (even if an error was returned after ping command).
			(*nodes_count)++;
			// Store address and reset board ID.
			nodes_list[node_list_idx].address = node_address;
			nodes_list[node_list_idx].board_id = DINFOX_BOARD_ID_ERROR;
			// Get board ID.
			read_params.node_address = node_address;
			status = AT_read_register(&read_params, &read_data, &read_status);
			if (status != NODE_SUCCESS) goto errors;
			// Check reply status.
			if (read_status.all == 0) {
				// Update board ID.
				nodes_list[node_list_idx].board_id = (uint8_t) read_data.value;
			}
			node_list_idx++;
			// Check index.
			if (node_list_idx >= nodes_list_size) break;
		}
		IWDG_reload();
	}
#else
	// Ping address.
	status = _AT_ping(&read_status);
	if (status != NODE_SUCCESS) goto errors;
	// Check reply status.
	if (read_status.all == 0) {
		// Node found (even if an error was returned after ping command).
		(*nodes_count)++;
		// Store address and reset board ID.
		nodes_list[node_list_idx].address = node_address;
		nodes_list[node_list_idx].board_id = DINFOX_BOARD_ID_ERROR;
		// Get board ID.
		status = AT_read_register(&read_params, &read_data, &read_status);
		if (status != NODE_SUCCESS) goto errors;
		// Check reply status.
		if (read_status.all == 0) {
			// Update board ID.
			nodes_list[node_list_idx].board_id = (uint8_t) read_data.value;
		}
	}
#endif
	return NODE_SUCCESS;
errors:
	return status;
}

/* FILL AT BUFFER WITH A NEW BYTE (CALLED BY LPUART INTERRUPT).
 * @param rx_byte:	Incoming byte.
 * @return:			None.
 */
void AT_fill_rx_buffer(uint8_t rx_byte) {
	// Read current index.
	uint8_t idx = at_ctx.reply[at_ctx.reply_write_idx].size;
	// Check ending characters.
	if (rx_byte == AT_FRAME_END) {
		// Set flag on current buffer.
		at_ctx.reply[at_ctx.reply_write_idx].buffer[idx] = STRING_CHAR_NULL;
		at_ctx.reply[at_ctx.reply_write_idx].line_end_flag = 1;
		// Switch buffer.
		at_ctx.reply_write_idx = (at_ctx.reply_write_idx + 1) % AT_REPLY_BUFFER_DEPTH;
		// Reset LBUS layer.
		LBUS_reset();
	}
	else {
		// Store incoming byte.
		at_ctx.reply[at_ctx.reply_write_idx].buffer[idx] = rx_byte;
		// Manage index.
		idx = (idx + 1) % AT_BUFFER_SIZE_BYTES;
		at_ctx.reply[at_ctx.reply_write_idx].size = idx;
	}
}

