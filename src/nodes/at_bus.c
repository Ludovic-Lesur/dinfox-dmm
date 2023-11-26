/*
 * at.c
 *
 *  Created on: Feb 18, 2023
 *      Author: ludo
 */

#include "at_bus.h"

#include "common_reg.h"
#include "dinfox.h"
#include "gpio.h"
#include "iwdg.h"
#include "lbus.h"
#include "lptim.h"
#include "lpuart.h"
#include "mapping.h"
#include "node.h"
#include "node_common.h"
#include "parser.h"
#include "string.h"

/*** AT local macros ***/

#define AT_BUS_FRAME_END				STRING_CHAR_CR

#define AT_BUS_BUFFER_SIZE_BYTES		128
#define AT_BUS_STRING_VALUE_BUFFER_SIZE	16
#define AT_BUS_REPLY_BUFFER_DEPTH		16

#define AT_BUS_REPLY_PARSING_DELAY_MS	20
#define AT_BUS_SEQUENCE_TIMEOUT_MS		120000

#define AT_BUS_COMMAND_WRITE_REGISTER	"AT$W="
#define AT_BUS_COMMAND_READ_REGISTER	"AT$R="
#define AT_BUS_COMMAND_SEPARATOR		","

#define AT_BUS_REPLY_OK					"OK"
#define AT_BUS_REPLY_ERROR				"ERROR"

/*** AT local structures ***/

/*******************************************************************/
typedef struct {
	volatile char_t buffer[AT_BUS_BUFFER_SIZE_BYTES];
	volatile uint8_t size;
	volatile uint8_t line_end_flag;
	PARSER_context_t parser;
} AT_BUS_reply_buffer_t;

/*******************************************************************/
typedef struct {
	// Command buffer.
	char_t command[AT_BUS_BUFFER_SIZE_BYTES];
	uint8_t command_size;
	// Response buffers.
	AT_BUS_reply_buffer_t reply[AT_BUS_REPLY_BUFFER_DEPTH];
	volatile uint8_t reply_write_idx;
	uint8_t reply_read_idx;
} AT_BUS_context_t;

/*** AT local global variables ***/

static AT_BUS_context_t at_bus_ctx;

/*** AT local functions ***/

/*******************************************************************/
static void _AT_BUS_fill_rx_buffer(uint8_t rx_byte) {
	// Read current index.
	uint8_t idx = at_bus_ctx.reply[at_bus_ctx.reply_write_idx].size;
	// Check ending characters.
	if (rx_byte == AT_BUS_FRAME_END) {
		// Set flag on current buffer.
		at_bus_ctx.reply[at_bus_ctx.reply_write_idx].buffer[idx] = STRING_CHAR_NULL;
		at_bus_ctx.reply[at_bus_ctx.reply_write_idx].line_end_flag = 1;
		// Switch buffer.
		at_bus_ctx.reply_write_idx = (at_bus_ctx.reply_write_idx + 1) % AT_BUS_REPLY_BUFFER_DEPTH;
		// Reset LBUS layer.
		LBUS_reset();
	}
	else {
		// Store incoming byte.
		at_bus_ctx.reply[at_bus_ctx.reply_write_idx].buffer[idx] = rx_byte;
		// Manage index.
		idx = (idx + 1) % AT_BUS_BUFFER_SIZE_BYTES;
		at_bus_ctx.reply[at_bus_ctx.reply_write_idx].size = idx;
	}
}

/*******************************************************************/
static void _AT_BUS_flush_command(void) {
	// Local variables.
	uint8_t idx = 0;
	// Flush buffer.
	for (idx=0 ; idx<AT_BUS_BUFFER_SIZE_BYTES ; idx++) at_bus_ctx.command[idx] = STRING_CHAR_NULL;
	at_bus_ctx.command_size = 0;
}

/*******************************************************************/
static void _AT_BUS_flush_reply(uint8_t reply_index) {
	// Flush buffer.
	at_bus_ctx.reply[reply_index].size = 0;
	// Reset flag.
	at_bus_ctx.reply[reply_index].line_end_flag = 0;
	// Reset parser.
	at_bus_ctx.reply[reply_index].parser.buffer = (char_t*) at_bus_ctx.reply[reply_index].buffer;
	at_bus_ctx.reply[reply_index].parser.buffer_size = 0;
	at_bus_ctx.reply[reply_index].parser.separator_idx = 0;
	at_bus_ctx.reply[reply_index].parser.start_idx = 0;
}

/*******************************************************************/
static void _AT_BUS_flush_replies(void) {
	// Local variabless.
	uint8_t rep_idx = 0;
	// Reset replys buffers.
	for (rep_idx=0 ; rep_idx<AT_BUS_REPLY_BUFFER_DEPTH ; rep_idx++) {
		_AT_BUS_flush_reply(rep_idx);
	}
	// Reset index and count.
	at_bus_ctx.reply_write_idx = 0;
	at_bus_ctx.reply_read_idx = 0;
}

/*******************************************************************/
static NODE_status_t _AT_BUS_wait_reply(NODE_reply_parameters_t* reply_params, uint32_t* reg_value, NODE_access_status_t* reply_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	PARSER_status_t parser_status = PARSER_SUCCESS;
	LPTIM_status_t lptim1_status = LPTIM_SUCCESS;
	uint32_t reply_time_ms = 0;
	uint32_t sequence_time_ms = 0;
	uint8_t reply_count = 0;
	// Check parameters.
	if ((reg_value == NULL) || (reply_status == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	if ((reply_params -> type) >= NODE_REPLY_TYPE_LAST) {
		status = NODE_ERROR_REPLY_TYPE;
		goto errors;
	}
	// Reset status.
	(reply_status -> all) = 0;
	// Directly exit function with success status for none reply type.
	if ((reply_params-> type) == NODE_REPLY_TYPE_NONE) goto errors;
	// Main reception loop.
	while (1) {
		// Delay.
		lptim1_status = LPTIM1_delay_milliseconds(AT_BUS_REPLY_PARSING_DELAY_MS, LPTIM_DELAY_MODE_STOP);
		LPTIM1_exit_error(NODE_ERROR_BASE_LPTIM);
		reply_time_ms += AT_BUS_REPLY_PARSING_DELAY_MS;
		sequence_time_ms += AT_BUS_REPLY_PARSING_DELAY_MS;
		// Check write index.
		if (at_bus_ctx.reply_write_idx != at_bus_ctx.reply_read_idx) {
			// Check line end flag.
			if (at_bus_ctx.reply[at_bus_ctx.reply_read_idx].line_end_flag != 0) {
				// Increment parsing count.
				reply_count++;
				// Reset time and flag.
				reply_time_ms = 0;
				at_bus_ctx.reply[at_bus_ctx.reply_read_idx].line_end_flag = 0;
				// Update buffer length.
				at_bus_ctx.reply[at_bus_ctx.reply_read_idx].parser.buffer_size = at_bus_ctx.reply[at_bus_ctx.reply_read_idx].size;
				// Parse reply.
				switch (reply_params -> type) {
				case NODE_REPLY_TYPE_OK:
					// Compare to reference string.
					parser_status = PARSER_compare(&at_bus_ctx.reply[at_bus_ctx.reply_read_idx].parser, PARSER_MODE_COMMAND, AT_BUS_REPLY_OK);
					break;
				case NODE_REPLY_TYPE_VALUE:
					// Parse register.
					parser_status = DINFOX_parse_register(&at_bus_ctx.reply[at_bus_ctx.reply_read_idx].parser, STRING_CHAR_NULL, reg_value);
					break;
				default:
					status = NODE_ERROR_REPLY_TYPE;
					break;
				}
				// Check status.
				if (parser_status == PARSER_SUCCESS) {
					// Update raw pointer, status and exit.
					(reply_status -> all) = 0;
					break;
				}
				// Check error.
				parser_status = PARSER_compare(&at_bus_ctx.reply[at_bus_ctx.reply_read_idx].parser, PARSER_MODE_HEADER, AT_BUS_REPLY_ERROR);
				if (parser_status == PARSER_SUCCESS) {
					// Update output data.
					(reply_status -> error_received) = 1;
					break;
				}
			}
			// Update read index.
			_AT_BUS_flush_reply(at_bus_ctx.reply_read_idx);
			at_bus_ctx.reply_read_idx = (at_bus_ctx.reply_read_idx + 1) % AT_BUS_REPLY_BUFFER_DEPTH;
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
		if (sequence_time_ms > AT_BUS_SEQUENCE_TIMEOUT_MS) {
			// Set status to timeout in any case.
			(reply_status -> sequence_timeout) = 1;
			break;
		}
		IWDG_reload();
	}
errors:
	return status;
}

/*** AT functions ***/

/*******************************************************************/
void AT_BUS_init(void) {
	// Init context.
	_AT_BUS_flush_command();
	_AT_BUS_flush_replies();
	// Init LBUS layer.
	LBUS_init(&_AT_BUS_fill_rx_buffer);
}

/*******************************************************************/
NODE_status_t AT_BUS_send_command(NODE_command_parameters_t* command_params) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	LBUS_status_t lbus_status = LBUS_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	// Flush buffer.
	_AT_BUS_flush_command();
	// Add command.
	string_status = STRING_append_string(at_bus_ctx.command, AT_BUS_BUFFER_SIZE_BYTES, (command_params -> command), &at_bus_ctx.command_size);
	STRING_exit_error(NODE_ERROR_BASE_STRING);
	// Add AT ending character.
	at_bus_ctx.command[at_bus_ctx.command_size++] = AT_BUS_FRAME_END;
	// Reset replies.
	_AT_BUS_flush_replies();
	// Disable receiver.
	LPUART1_disable_rx();
	// Send command.
	lbus_status = LBUS_send((command_params -> node_addr), (uint8_t*) at_bus_ctx.command, at_bus_ctx.command_size);
	LBUS_exit_error(NODE_ERROR_BASE_LBUS);
	// Enable receiver.
	LPUART1_enable_rx();
errors:
	return status;
}

/*******************************************************************/
NODE_status_t AT_BUS_write_register(NODE_access_parameters_t* write_params, uint32_t reg_value, uint32_t reg_mask, NODE_access_status_t* write_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	NODE_command_parameters_t command_params;
	char_t command[AT_BUS_BUFFER_SIZE_BYTES] = {STRING_CHAR_NULL};
	uint8_t command_size = 0;
	char_t str_value[AT_BUS_STRING_VALUE_BUFFER_SIZE];
	uint32_t unused_reg_value = 0;
	// Check parameters.
	if ((write_params == NULL) || (write_status == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Build command structure.
	command_params.node_addr = (write_params -> node_addr);
	command_params.command = (char_t*) command;
	// Build write command.
	string_status = STRING_append_string(command, AT_BUS_BUFFER_SIZE_BYTES, AT_BUS_COMMAND_WRITE_REGISTER, &command_size);
	STRING_exit_error(NODE_ERROR_BASE_STRING);
	string_status = DINFOX_register_to_string((uint32_t) (write_params -> reg_addr), str_value);
	STRING_exit_error(NODE_ERROR_BASE_STRING);
	string_status = STRING_append_string(command, AT_BUS_BUFFER_SIZE_BYTES, str_value, &command_size);
	STRING_exit_error(NODE_ERROR_BASE_STRING);
	string_status = STRING_append_string(command, AT_BUS_BUFFER_SIZE_BYTES, AT_BUS_COMMAND_SEPARATOR, &command_size);
	STRING_exit_error(NODE_ERROR_BASE_STRING);
	string_status = DINFOX_register_to_string(reg_value, str_value);
	STRING_exit_error(NODE_ERROR_BASE_STRING);
	string_status = STRING_append_string(command, AT_BUS_BUFFER_SIZE_BYTES, str_value, &command_size);
	STRING_exit_error(NODE_ERROR_BASE_STRING);
	// Add mask if needed.
	if (reg_mask != DINFOX_REG_MASK_ALL) {
		string_status = STRING_append_string(command, AT_BUS_BUFFER_SIZE_BYTES, AT_BUS_COMMAND_SEPARATOR, &command_size);
		STRING_exit_error(NODE_ERROR_BASE_STRING);
		string_status = DINFOX_register_to_string(reg_mask, str_value);
		STRING_exit_error(NODE_ERROR_BASE_STRING);
		string_status = STRING_append_string(command, AT_BUS_BUFFER_SIZE_BYTES, str_value, &command_size);
		STRING_exit_error(NODE_ERROR_BASE_STRING);
	}
	// Send command.
	status = AT_BUS_send_command(&command_params);
	// Wait reply.
	status = _AT_BUS_wait_reply(&(write_params -> reply_params), &unused_reg_value, write_status);
	if (status != NODE_SUCCESS) goto errors;
errors:
	LPUART1_disable_rx();
	return status;
}

/*******************************************************************/
NODE_status_t AT_BUS_read_register(NODE_access_parameters_t* read_params, uint32_t* reg_value, NODE_access_status_t* read_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	NODE_command_parameters_t command_params;
	char_t command[AT_BUS_BUFFER_SIZE_BYTES] = {STRING_CHAR_NULL};
	uint8_t command_size = 0;
	// Check parameters.
	if ((read_params == NULL) || (read_status == NULL) || (reg_value == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Build command structure.
	command_params.node_addr = (read_params -> node_addr);
	command_params.command = (char_t*) command;
	// Build read command.
	string_status = STRING_append_string(command, AT_BUS_BUFFER_SIZE_BYTES, AT_BUS_COMMAND_READ_REGISTER, &command_size);
	STRING_exit_error(NODE_ERROR_BASE_STRING);
	string_status = STRING_append_value(command, AT_BUS_BUFFER_SIZE_BYTES, (read_params -> reg_addr), STRING_FORMAT_HEXADECIMAL, 0, &command_size);
	STRING_exit_error(NODE_ERROR_BASE_STRING);
	// Send command.
	status = AT_BUS_send_command(&command_params);
	if (status != NODE_SUCCESS) goto errors;
	// Wait reply.
	status = _AT_BUS_wait_reply(&(read_params -> reply_params), reg_value, read_status);
	if (status != NODE_SUCCESS) goto errors;
errors:
	LPUART1_disable_rx();
	return status;
}

/*******************************************************************/
NODE_status_t AT_BUS_scan(NODE_t* nodes_list, uint8_t nodes_list_size, uint8_t* nodes_count) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	NODE_access_parameters_t read_params;
	NODE_access_status_t read_status;
	NODE_address_t node_addr = 0;
	uint32_t reg_value = 0;
	// Check parameters.
	if ((nodes_list == NULL) || (nodes_count == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Reset count.
	(*nodes_count) = 0;
	// Build read input common parameters.
	read_params.reg_addr = COMMON_REG_ADDR_NODE_ID;
	read_params.reply_params.timeout_ms = AT_BUS_DEFAULT_TIMEOUT_MS;
	read_params.reply_params.type = NODE_REPLY_TYPE_VALUE;
	// Loop on all addresses.
	for (node_addr=0 ; node_addr<=DINFOX_NODE_ADDRESS_LBUS_LAST ; node_addr++) {
		// Uppdate address.
		read_params.node_addr = node_addr;
		// Read NODE_ID register.
		status = AT_BUS_read_register(&read_params, &reg_value, &read_status);
		if (status != NODE_SUCCESS) goto errors;
		// Check reply status.
		if (read_status.all == 0) {
			// Check node address consistency.
			if (DINFOX_read_field(reg_value, COMMON_REG_NODE_ID_MASK_NODE_ADDR) == node_addr) {
				// Update board ID.
				nodes_list[(*nodes_count)].address = DINFOX_read_field(reg_value, COMMON_REG_NODE_ID_MASK_NODE_ADDR);
				nodes_list[(*nodes_count)].board_id = DINFOX_read_field(reg_value, COMMON_REG_NODE_ID_MASK_BOARD_ID);
				// Update nodes count.
				(*nodes_count)++;
			}
		}
		// Check index.
		if ((*nodes_count) >= nodes_list_size) break;
		IWDG_reload();
	}
	return NODE_SUCCESS;
errors:
	return status;
}
