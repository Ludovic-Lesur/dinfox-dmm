/*
 * lvrm.c
 *
 *  Created on: 22 jan. 2023
 *      Author: Ludo
 */

#include "dinfox.h"
#include "lvrm.h"
#include "mode.h"
#include "node_common.h"
#include "rs485.h"
#include "rs485_common.h"
#include "string.h"

/*** LVRM local structures ***/

typedef enum {
	LVRM_REGISTER_VCOM_MV = DINFOX_REGISTER_LAST,
	LVRM_REGISTER_VOUT_MV,
	LVRM_REGISTER_IOUT_UA,
	LVRM_REGISTER_OUT_EN,
	LVRM_REGISTER_LAST,
} LVRM_register_address_t;

typedef enum {
	LVRM_STRING_DATA_INDEX_VCOM_MV = DINFOX_STRING_DATA_INDEX_LAST,
	LVRM_STRING_DATA_INDEX_VOUT_MV,
	LVRM_STRING_DATA_INDEX_IOUT_UA,
	LVRM_STRING_DATA_INDEX_OUT_EN,
	LVRM_STRING_DATA_INDEX_LAST,
} LVRM_measurement_index_t;

/*** LVRM local macros ***/

static const char_t* LVRM_BOARD_NAME = "LVRM";

static const char_t* LVRM_STRING_DATA_NAME[LVRM_STRING_DATA_INDEX_LAST] = {DINFOX_COMMON_STRING_DATA_NAME, "VCOM =", "VOUT =", "IOUT =", "RELAY ="};
static const char_t* LVRM_STRING_DATA_UNIT[LVRM_STRING_DATA_INDEX_LAST] = {DINFOX_COMMON_STRING_DATA_UNIT, "mV", "mV", "uA", STRING_NULL};

static const STRING_format_t LVRM_REGISTER_FORMAT[LVRM_REGISTER_LAST] = {DINFOX_COMMON_REGISTER_FORMAT, STRING_FORMAT_DECIMAL, STRING_FORMAT_DECIMAL, STRING_FORMAT_DECIMAL, STRING_FORMAT_BOOLEAN};

/*** LVRM local structures ***/

typedef struct {
	RS485_address_t rs485_address;
	char_t string_data_value[LVRM_STRING_DATA_INDEX_LAST][DINFOX_STRING_BUFFER_SIZE];
	uint8_t string_data_value_size[LVRM_STRING_DATA_INDEX_LAST];
	int32_t integer_data_value[LVRM_REGISTER_LAST];
} LVRM_context_t;

/*** LVRM local global variables ***/

static LVRM_context_t lvrm_ctx;

/*** LVRM local functions ***/

/* GENERIC MACRO TO APPEND A STRING TO THE DATAS VALUE BUFFER.
 * @param measurement_idx:	Measurement line index.
 * @param str:				String to append.
 * @return:					None.
 */
#define _LVRM_append_string(str) { \
	string_status = STRING_append_string(lvrm_ctx.string_data_value[string_data_index], DINFOX_STRING_BUFFER_SIZE, str, &(lvrm_ctx.string_data_value_size[string_data_index])); \
	STRING_status_check(NODE_ERROR_BASE_STRING); \
}

/* GENERIC MACRO TO SET A DATA VALUE TO ERROR.
 * @param measurement_idx:	Measurement line index.
 * @return:					None.
 */
#define _LVRM_set_error() { \
		_LVRM_flush_string_data_value(string_data_index); \
	_LVRM_append_string(DINFOX_STRING_DATA_ERROR); \
}

/* FLUSH ONE LINE OF THE MEASURERMENTS VALUE BUFFER.
 * @param:	None.
 * @return:	None.
 */
void _LVRM_flush_string_data_value(uint8_t measurement_index) {
	// Local variables.
	uint8_t idx = 0;
	// Char loop.
	for (idx=0 ; idx<DINFOX_STRING_BUFFER_SIZE ; idx++) lvrm_ctx.string_data_value[measurement_index][idx] = STRING_CHAR_NULL;
	lvrm_ctx.string_data_value_size[measurement_index] = 0;
}

/* FLUSH WHOLE DATAS VALUE BUFFER.
 * @param:	None.
 * @return:	None.
 */
void _LVRM_flush_all_data_value(void) {
	// Local variables.
	uint8_t idx = 0;
	// String data.
	for (idx=0 ; idx<LVRM_STRING_DATA_INDEX_LAST ; idx++) _LVRM_flush_string_data_value(idx);
	// Integer data.
	for (idx=0 ; idx<LVRM_REGISTER_LAST ; idx++) lvrm_ctx.integer_data_value[idx] = 0;
}

/*** LVRM functions ***/

/* GET NODE BOARD NAME.
 * @param board_name:	Pointer to string that will contain board name.
 * @return status:		Function execution status.
 */
NODE_status_t LVRM_get_board_name(char_t** board_name_ptr) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	// Update pointer.
	(*board_name_ptr) = (char_t*) LVRM_BOARD_NAME;
	return status;
}

/* SET LVRM NODE ADDRESS.
 * @param rs485_address:	RS485 address of the LVRM node.
 * @return:					None.
 */
NODE_status_t LVRM_set_rs485_address(RS485_address_t rs485_address) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	// Check address.
	if (rs485_address > RS485_ADDRESS_LAST) {
		status = NODE_ERROR_RS485_ADDRESS;
		goto errors;
	}
	lvrm_ctx.rs485_address = rs485_address;
errors:
	return status;
}

/* RETRIEVE DATA FROM LVRM NODE.
 * @param string_data_index:	Node string data index.
 * @return status:				Function execution status.
 */
NODE_status_t LVRM_update_data(uint8_t string_data_index) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	RS485_status_t rs485_status = RS485_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	RS485_read_input_t read_input;
	RS485_reply_t reply;
	uint8_t register_address = 0;
	// Common read input parameters.
#ifdef AM
	read_input.node_address = lvrm_ctx.rs485_address;
#endif
	read_input.type = RS485_REPLY_TYPE_VALUE;
	read_input.timeout_ms = DINFOX_RS485_TIMEOUT_MS;
	read_input.format = STRING_FORMAT_DECIMAL;
	// Check index.
	if (string_data_index >= LVRM_STRING_DATA_INDEX_LAST) {
		status = NODE_ERROR_STRING_DATA_INDEX;
		goto errors;
	}
	// Flush line.
	_LVRM_flush_string_data_value(string_data_index);
	// Common data.
	if (string_data_index < DINFOX_STRING_DATA_INDEX_LAST) {
		status = DINFOX_update_data(lvrm_ctx.rs485_address, string_data_index, lvrm_ctx.string_data_value[string_data_index], &(lvrm_ctx.string_data_value_size[string_data_index]), &(lvrm_ctx.integer_data_value[string_data_index]));
		if (status != NODE_SUCCESS) goto errors;
	}
	// Specific data.
	else {
		// Convert to register address.
		register_address = (string_data_index + DINFOX_REGISTER_LAST - DINFOX_STRING_DATA_INDEX_LAST);
		read_input.register_address = register_address;
		// Read data.
		rs485_status = RS485_read_register(&read_input, &reply);
		RS485_status_check(NODE_ERROR_BASE_RS485);
		// Check reply.
		if (reply.status.all == 0) {
			// Update integer data.
			lvrm_ctx.integer_data_value[register_address] = reply.value;
			// Specific print for relay.
			if (string_data_index == LVRM_STRING_DATA_INDEX_OUT_EN) {
				if (lvrm_ctx.integer_data_value[register_address] == 0) {
					_LVRM_append_string("OFF");
				}
				else {
					_LVRM_append_string("ON");
				}
			}
			else {
				_LVRM_append_string(reply.raw);
			}
			_LVRM_append_string((char_t*) LVRM_STRING_DATA_UNIT[string_data_index]);
		}
		else {
			_LVRM_set_error();
		}
	}
errors:
	return status;
}

/* RETRIEVE DATA FROM LVRM NODE.
 * @param:			None.
 * @return status:	Function execution status.
 */
NODE_status_t LVRM_update_all_data(void) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	uint8_t idx = 0;
	// Reset buffers.
	_LVRM_flush_all_data_value();
	// String data loop.
	for (idx=0 ; idx<LVRM_STRING_DATA_INDEX_LAST ; idx++) {
		status = LVRM_update_data(idx);
		if (status != NODE_SUCCESS) goto errors;
	}
errors:
	return status;
}

/* UNSTACK NODE DATA FORMATTED AS STRING.
 * @param string_data_name_ptr:		Pointer to string that will contain next measurement name.
 * @param string_data_value_ptr:	Pointer to string that will contain next measurement value.
 * @return status:					Function execution status.
 */
NODE_status_t LVRM_read_string_data(uint8_t string_data_index, char_t** string_data_name_ptr, char_t** string_data_value_ptr) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	// Check index.
	if (string_data_index < LVRM_STRING_DATA_INDEX_LAST) {
		// Update input pointers.
		(*string_data_name_ptr) = (char_t*) LVRM_STRING_DATA_NAME[string_data_index];
		(*string_data_value_ptr) = (char_t*) lvrm_ctx.string_data_value[string_data_index];
	}
	else {
		// Return NULL pointers.
		(*string_data_name_ptr) = NULL;
		(*string_data_value_ptr) = NULL;
	}
	return status;
}

/* WRITE LVRM NODE DATA.
 * @param string_data_index:	Node string data index.
 * @param value:				Value to write in corresponding register.
 * @param write_status:			Pointer to the writing operation status.
 * @return status:				Function execution status.
 */
NODE_status_t LVRM_write_data(uint8_t string_data_index, uint8_t value, RS485_reply_status_t* write_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	RS485_status_t rs485_status = RS485_SUCCESS;
	RS485_write_input_t write_input;
	RS485_reply_t reply;
	uint8_t register_address = 0;
	// Convert string data index to register.
	if (string_data_index >= DINFOX_STRING_DATA_INDEX_LAST) {
		register_address = (string_data_index + DINFOX_REGISTER_LAST - DINFOX_STRING_DATA_INDEX_LAST);
	}
	// Common write input parameters.
#ifdef AM
	write_input.node_address = lvrm_ctx.rs485_address;
#endif
	write_input.value = value;
	write_input.timeout_ms = DINFOX_RS485_TIMEOUT_MS;
	write_input.format = LVRM_REGISTER_FORMAT[register_address];
	write_input.register_address = register_address;
	// Check writable registers.
	rs485_status = RS485_write_register(&write_input, &reply);
	RS485_status_check(NODE_ERROR_BASE_RS485);
	// Check reply.
	(*write_status).all = reply.status.all;
errors:
	return status;
}

/* GET NODE SIGFOX PAYLOAD.
 * @param ul_payload:		Pointer that will contain UL payload of the node.
 * @param ul_payload_size:	Pointer to byte that will contain UL payload size.
 * @return status:			Function execution status.
 */
NODE_status_t LVRM_get_sigfox_payload(uint8_t* ul_payload, uint8_t* ul_payload_size) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	return status;
}
