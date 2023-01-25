/*
 * rs485.c
 *
 *  Created on: 28 oct 2022
 *      Author: Ludo
 */

#include "rs485.h"

#include "dinfox.h"
#include "gpio.h"
#include "iwdg.h"
#include "lptim.h"
#include "lpuart.h"
#include "mapping.h"
#include "mode.h"
#include "rs485_common.h"
#include "string.h"

/*** RS485 local macros ***/

#define RS485_BUFFER_SIZE_BYTES			80
#define RS485_REPLY_BUFFER_DEPTH		64

#define RS485_REPLY_PARSING_DELAY_MS	10
#define RS485_REPLY_TIMEOUT_MS			100
#define RS485_SEQUENCE_TIMEOUT_MS		1000

#define RS485_COMMAND_WRITE_REGISTER	"RS$W="
#define RS485_COMMAND_READ_REGISTER		"RS$R="

#define RS485_REPLY_OK					"OK"
#define RS485_REPLY_ERROR				"ERROR"

/*** RS485 local structures ***/

typedef struct {
	volatile char_t buffer[RS485_BUFFER_SIZE_BYTES];
	volatile uint8_t size;
	volatile uint8_t line_end_flag;
	PARSER_context_t parser;
} RS485_reply_buffer_t;

typedef struct {
	// Command buffer.
	char_t command[RS485_BUFFER_SIZE_BYTES];
	uint8_t command_size;
#ifdef AM
	RS485_address_t expected_slave_address;
#endif
	// Response buffers.
	RS485_reply_buffer_t reply[RS485_REPLY_BUFFER_DEPTH];
	volatile uint8_t reply_write_idx;
	uint8_t reply_read_idx;
} RS485_context_t;

/*** RS485 local global variables ***/

static RS485_context_t rs485_ctx;

/*** RS485 local functions ***/

/* FLUSH COMMAND BUFFER.
 * @param:	None.
 * @return:	None.
 */
static void _RS485_flush_command(void) {
	// Local variables.
	uint8_t idx = 0;
	// Flush buffer.
	for (idx=0 ; idx<RS485_BUFFER_SIZE_BYTES ; idx++) rs485_ctx.command[idx] = STRING_CHAR_NULL;
	rs485_ctx.command_size = 0;
}

/* FLUSH RS485 REPLY BUFFER.
 * @param reply_index:	Reply index to reset.
 * @return:				None.
 */
static void _RS485_flush_reply(uint8_t reply_index) {
	// Flush buffer.
	rs485_ctx.reply[reply_index].size = 0;
	// Reset flag.
	rs485_ctx.reply[reply_index].line_end_flag = 0;
	// Reset parser.
	rs485_ctx.reply[reply_index].parser.buffer = (char_t*) rs485_ctx.reply[reply_index].buffer;
	rs485_ctx.reply[reply_index].parser.buffer_size = 0;
	rs485_ctx.reply[reply_index].parser.separator_idx = 0;
	rs485_ctx.reply[reply_index].parser.start_idx = 0;
}

/* FLUSH ALL RS485 REPLY BUFFERS.
 * @param:	None.
 * @return:	None.
 */
static void _RS485_flush_replies(void) {
	// Local variabless.
	uint8_t rep_idx = 0;
	// Reset replys buffers.
	for (rep_idx=0 ; rep_idx<RS485_REPLY_BUFFER_DEPTH ; rep_idx++) {
		_RS485_flush_reply(rep_idx);
	}
	// Reset index and count.
	rs485_ctx.reply_write_idx = 0;
	rs485_ctx.reply_read_idx = 0;
}

#ifdef AM
/* SEND THE COMMAND BUFFER OVER RS485 BUS.
 * @param slave_address:	RS485 address of the destination board.
 * @return status:			Function execution status.
 */
static RS485_status_t _RS485_send(RS485_address_t slave_address) {
#else
/* SEND THE COMMAND BUFFER OVER RS485 BUS.
 * @param:			None.
 * @return status:	Function execution status.
 */
static RS485_status_t _RS485_send(void) {
#endif
	// Local variables.
	RS485_status_t status = RS485_SUCCESS;
	LPUART_status_t lpuart1_status = LPUART_SUCCESS;
	// Add RS485 ending character.
	rs485_ctx.command[rs485_ctx.command_size++] = RS485_FRAME_END;
	rs485_ctx.command[rs485_ctx.command_size++] = STRING_CHAR_NULL;
	// Reset replies.
	_RS485_flush_replies();
#ifdef AM
	// Store slave address to authenticate next data reception.
	rs485_ctx.expected_slave_address = slave_address;
#endif
	// Send command.
	LPUART1_disable_rx();
#ifdef AM
	lpuart1_status = LPUART1_send_command(slave_address, rs485_ctx.command);
#else
	lpuart1_status = LPUART1_send_command(rs485_ctx.command);
#endif
	LPUART1_enable_rx();
	LPUART1_status_check(RS485_ERROR_BASE_LPUART);
errors:
	return status;
}

/*** RS485 functions ***/

/* INIT RS485 INTERFACE.
 * @param:	None.
 * @return:	None.
 */
void RS485_init(void) {
	// Reset buffers.
	_RS485_flush_command();
	_RS485_flush_replies();
	// Reset node list.
	uint8_t idx = 0;
	rs485_common_ctx.nodes_count = 0;
	for (idx=0 ; idx<RS485_NODES_LIST_SIZE_MAX ; idx++) {
		rs485_common_ctx.nodes_list[idx].address = (RS485_ADDRESS_LAST + 1);
		rs485_common_ctx.nodes_list[idx].board_id = DINFOX_BOARD_ID_ERROR;
	}
}

#ifdef AM
/* SEND A COMMAND TO AN RS485 NODE.
 * @param slave_address:	RS485 address of the destination board.
 * @param command:			Command to send.
 * @return status:			Function execution status.
 */
RS485_status_t RS485_send_command(RS485_address_t slave_address, char_t* command) {
#else
/* SEND A COMMAND OVER RS485 BUS.
 * @param command:	Command to send.
 * @return status:	Function execution status.
 */
RS485_status_t RS485_send_command(char_t* command) {
#endif
	// Local variables.
	RS485_status_t status = RS485_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	// Check parameters.
	if (command == NULL) {
		status = RS485_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Flush command buffer.
	_RS485_flush_command();
	// Copy command into local buffer.
	string_status = STRING_append_string(rs485_ctx.command, RS485_BUFFER_SIZE_BYTES, command, &rs485_ctx.command_size);
	STRING_status_check(RS485_ERROR_BASE_STRING);
	// Send command.
#ifdef AM
	status = _RS485_send(slave_address);
#else
	status = _RS485_send();
#endif
errors:
	return status;
}

/* WAIT FOR RECEIVING A VALUE.
 * @param reply_in_ptr:		Pointer to the reply input parameters.
 * @param reply_out_ptr:	Pointer to the reply output data.
 * @return status:			Function execution status.
 */
RS485_status_t RS485_wait_reply(RS485_reply_input_t* reply_in_ptr, RS485_reply_output_t* reply_out_ptr) {
	// Local variables.
	RS485_status_t status = RS485_SUCCESS;
	PARSER_status_t parser_status = PARSER_SUCCESS;
	LPTIM_status_t lptim1_status = LPTIM_SUCCESS;
	uint32_t reply_time_ms = 0;
	uint32_t sequence_time_ms = 0;
	uint8_t reply_count = 0;
	// Check parameters.
	if ((reply_in_ptr == NULL) || (reply_out_ptr == NULL)) {
		status = RS485_ERROR_NULL_PARAMETER;
		goto errors;
	}
	if ((reply_in_ptr -> type) >= RS485_REPLY_TYPE_LAST) {
		status = RS485_ERROR_REPLY_TYPE;
		goto errors;
	}
	// Reset output data.
	(reply_out_ptr -> value) = 0;
	(reply_out_ptr -> status).all = 0;
	(reply_out_ptr -> raw) = NULL;
	// Main reception loop.
	while (1) {
		// Delay.
		lptim1_status = LPTIM1_delay_milliseconds(RS485_REPLY_PARSING_DELAY_MS, 0);
		LPTIM1_status_check(RS485_ERROR_BASE_LPTIM);
		reply_time_ms += RS485_REPLY_PARSING_DELAY_MS;
		sequence_time_ms += RS485_REPLY_PARSING_DELAY_MS;
		// Check write index.
		if (rs485_ctx.reply_write_idx != rs485_ctx.reply_read_idx) {
			// Check line end flag.
			if (rs485_ctx.reply[rs485_ctx.reply_read_idx].line_end_flag != 0) {
				// Increment parsing count.
				reply_count++;
				// Reset time and flag.
				reply_time_ms = 0;
				rs485_ctx.reply[rs485_ctx.reply_read_idx].line_end_flag = 0;
#ifdef AM
				// Check source address.
				if (rs485_ctx.reply[rs485_ctx.reply_read_idx].buffer[RS485_FRAME_FIELD_INDEX_SOURCE_ADDRESS] != rs485_ctx.expected_slave_address) {
					(reply_out_ptr -> status).source_address_mismatch = 1;
					continue;
				}
				// Skip source address before parsing.
				rs485_ctx.reply[rs485_ctx.reply_read_idx].parser.buffer = (char_t*) &(rs485_ctx.reply[rs485_ctx.reply_read_idx].buffer[RS485_FRAME_FIELD_INDEX_DATA]);
				rs485_ctx.reply[rs485_ctx.reply_read_idx].parser.buffer_size = (rs485_ctx.reply[rs485_ctx.reply_read_idx].size > 0) ? (rs485_ctx.reply[rs485_ctx.reply_read_idx].size - RS485_FRAME_FIELD_INDEX_DATA) : 0;
#else
				// Update buffer length.
				rs485_ctx.reply[rs485_ctx.reply_read_idx].parser.buffer_size = rs485_ctx.reply[rs485_ctx.reply_read_idx].size;
#endif
				// Parse reply.
				switch (reply_in_ptr -> type) {
				case RS485_REPLY_TYPE_RAW:
					// Do not parse.
					parser_status = PARSER_SUCCESS;
					break;
				case RS485_REPLY_TYPE_OK:
					// Compare to reference string.
					parser_status = PARSER_compare(&rs485_ctx.reply[rs485_ctx.reply_read_idx].parser, PARSER_MODE_COMMAND, RS485_REPLY_OK);
					break;
				case RS485_REPLY_TYPE_VALUE:
					// Parse value.
					parser_status = PARSER_get_parameter(&rs485_ctx.reply[rs485_ctx.reply_read_idx].parser, (reply_in_ptr -> format), STRING_CHAR_NULL, &(reply_out_ptr -> value));
					break;
				default:
					break;
				}
				// Check status.
				if (parser_status == PARSER_SUCCESS) {
					// Update raw pointer, status and exit..
					(reply_out_ptr -> status).all = 0;
#ifdef AM
					(reply_out_ptr -> raw) = (char_t*) (&(rs485_ctx.reply[rs485_ctx.reply_read_idx].buffer[RS485_FRAME_FIELD_INDEX_DATA]));
#else
					(reply_out_ptr -> raw) = (char_t*) (rs485_ctx.reply[rs485_ctx.reply_read_idx].buffer);
#endif
					break;
				}
				// Check error.
				parser_status = PARSER_compare(&rs485_ctx.reply[rs485_ctx.reply_read_idx].parser, PARSER_MODE_HEADER, RS485_REPLY_ERROR);
				if (parser_status == PARSER_SUCCESS) {
					// Update output data.
					(reply_out_ptr -> status).error_received = 1;
					break;
				}
			}
			// Update read index.
			_RS485_flush_reply(rs485_ctx.reply_read_idx);
			rs485_ctx.reply_read_idx = (rs485_ctx.reply_read_idx + 1) % RS485_REPLY_BUFFER_DEPTH;
		}
		// Exit if timeout.
		if (reply_time_ms > (reply_in_ptr -> timeout_ms)) {
			// Set status to timeout if none reply has been received, otherwise the parser error code is returned.
			if (reply_count == 0) {
				(reply_out_ptr -> status).reply_timeout = 1;
			}
			else {
				(reply_out_ptr -> status).parser_error = 1;
			}
			break;
		}
		if (sequence_time_ms > RS485_SEQUENCE_TIMEOUT_MS) {
			// Set status to timeout in any case.
			(reply_out_ptr -> status).sequence_timeout = 1;
			break;
		}
	}
errors:
	return status;
}

/* SCAN ALL NODES ON RS485 BUS.
 * @param:			None.
 * @return status:	Function execution status.
 */
RS485_status_t RS485_scan_nodes(void) {
	// Local variables.
	RS485_status_t status = RS485_SUCCESS;
	RS485_reply_input_t reply_in;
	RS485_reply_output_t reply_out;
	RS485_address_t node_address = 0;
	uint8_t node_list_idx = 0;
	NVM_status_t nvm_status = NVM_SUCCESS;
	// Check parameters.
	if (rs485_common_ctx.nodes_list == NULL) {
		status = RS485_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Read RS485 address in NVM.
	nvm_status = NVM_read_byte(NVM_ADDRESS_RS485_ADDRESS, &node_address);
	NVM_status_check(RS485_ERROR_BASE_NVM);
	// Add master board to the list.
	rs485_common_ctx.nodes_list[0].board_id = DINFOX_BOARD_ID_DMM;
	rs485_common_ctx.nodes_list[0].address = node_address;
	rs485_common_ctx.nodes_count = 1;
	node_list_idx = 1;
	// Build reply input common parameters.
	reply_in.format = STRING_FORMAT_HEXADECIMAL;
	reply_in.timeout_ms = RS485_REPLY_TIMEOUT_MS;
#ifdef AM
	// Loop on all addresses.
	for (node_address=0 ; node_address<=RS485_ADDRESS_LAST ; node_address++) {
		// Expect OK.
		reply_in.type = RS485_REPLY_TYPE_OK;
		// Send ping command.
		status = RS485_send_command(node_address, "RS");
		if (status != RS485_SUCCESS) goto errors;
		// Wait reply.
		status = RS485_wait_reply(&reply_in, &reply_out);
		if (status != RS485_SUCCESS) goto errors;
		// Check reply status.
		if (reply_out.status.all == 0) {
			// Node found (even if an error was returned after ping command).
			rs485_common_ctx.nodes_count++;
			// Store address and reset board ID.
			if (node_list_idx < RS485_NODES_LIST_SIZE_MAX) {
				rs485_common_ctx.nodes_list[node_list_idx].address = node_address;
				rs485_common_ctx.nodes_list[node_list_idx].board_id = DINFOX_BOARD_ID_ERROR;
			}
			// Expect value.
			reply_in.type = RS485_REPLY_TYPE_VALUE;
			// Get board ID.
			status = RS485_read_register(node_address, DINFOX_REGISTER_BOARD_ID, &reply_in, &reply_out);
			if (status != RS485_SUCCESS) goto errors;
			// Check reply status.
			if (reply_out.status.all == 0) {
				// Update board ID.
				rs485_common_ctx.nodes_list[node_list_idx].board_id = (uint8_t) reply_out.value;
			}
			node_list_idx++;
		}
		IWDG_reload();
	}
#else
	// Expect OK.
	reply_in.type = RS485_REPLY_TYPE_OK;
	// Send ping command.
	status = RS485_send_command("RS");
	if (status != RS485_SUCCESS) goto errors;
	// Wait reply.
	status = RS485_wait_reply(&reply_in, &reply_out);
	if (status != RS485_SUCCESS) goto errors;
	// Check reply status.
	if (reply_out.status.all == 0) {
		// Node found (even if an error was returned after ping command).
		rs485_common_ctx.nodes_count++;
		// Store address and reset board ID.
		if (node_list_idx < RS485_NODES_LIST_SIZE_MAX) {
			rs485_common_ctx.nodes_list[node_list_idx].address = node_address;
			rs485_common_ctx.nodes_list[node_list_idx].board_id = DINFOX_BOARD_ID_ERROR;
		}
		// Expect value.
		reply_in.type = RS485_REPLY_TYPE_VALUE;
		// Get board ID.
		status = RS485_read_register(DINFOX_REGISTER_BOARD_ID);
		if (status != RS485_SUCCESS) goto errors;
		// Wait reply.
		status = RS485_wait_reply(&reply_in, &reply_out);
		if (status != RS485_SUCCESS) goto errors;
		// Check reply status.
		if (reply_out.status.all == 0) {
			// Update board ID.
			rs485_common_ctx.nodes_list[node_list_idx].board_id = (uint8_t) reply_out.value;
		}
	}
#endif
	return RS485_SUCCESS;
errors:
	return status;
}

#ifdef AM
/* READ A REGISTER OF AN RS485 NODE.
 * @param slave_address:	RS485 address of the destination board.
 * @param register_address:	Register to read.
 * @param reply_in_ptr:		Pointer to the reply input parameters.
 * @param reply_out_ptr:	Pointer to the reply output data.
 * @return:					None.
 */
RS485_status_t RS485_read_register(RS485_address_t slave_address, uint8_t register_address, RS485_reply_input_t* reply_in_ptr, RS485_reply_output_t* reply_out_ptr) {
#else
/* READ A REGISTER OF AN RS485 NODE.
 * @param register_address:	Register to read.
 * @return:					None.
 */
RS485_status_t RS485_read_register(uint8_t register_address, RS485_reply_input_t* reply_in_ptr, RS485_reply_output_t* reply_out_ptr) {
#endif
	// Local variables.
	RS485_status_t status = RS485_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	// Flush command buffer.
	_RS485_flush_command();
	// Build read command.
	string_status = STRING_append_string(rs485_ctx.command, RS485_BUFFER_SIZE_BYTES, RS485_COMMAND_READ_REGISTER, &rs485_ctx.command_size);
	STRING_status_check(RS485_ERROR_BASE_STRING);
	string_status = STRING_append_value(rs485_ctx.command, RS485_BUFFER_SIZE_BYTES, register_address, STRING_FORMAT_HEXADECIMAL, 0, &rs485_ctx.command_size);
	STRING_status_check(RS485_ERROR_BASE_STRING);
	// Send command.
#ifdef AM
	status = _RS485_send(slave_address);
#else
	status = _RS485_send();
#endif
	if (status != RS485_SUCCESS) goto errors;
	// Wait reply.
	status = RS485_wait_reply(reply_in_ptr, reply_out_ptr);
errors:
	return status;
}

/* FILL RS485 BUFFER WITH A NEW BYTE (CALLED BY LPUART INTERRUPT).
 * @param rx_byte:	Incoming byte.
 * @return:			None.
 */
void RS485_fill_rx_buffer(uint8_t rx_byte) {
	// Read current index.
	uint8_t idx = rs485_ctx.reply[rs485_ctx.reply_write_idx].size;
	// Check ending characters.
	if (rx_byte == RS485_FRAME_END) {
		// Set flag on current buffer.
		rs485_ctx.reply[rs485_ctx.reply_write_idx].buffer[idx] = STRING_CHAR_NULL;
		rs485_ctx.reply[rs485_ctx.reply_write_idx].line_end_flag = 1;
		// Switch buffer.
		rs485_ctx.reply_write_idx = (rs485_ctx.reply_write_idx + 1) % RS485_REPLY_BUFFER_DEPTH;
	}
	else {
		// Store incoming byte.
		rs485_ctx.reply[rs485_ctx.reply_write_idx].buffer[idx] = rx_byte;
		// Manage index.
		idx = (idx + 1) % RS485_BUFFER_SIZE_BYTES;
		rs485_ctx.reply[rs485_ctx.reply_write_idx].size = idx;
	}
}
