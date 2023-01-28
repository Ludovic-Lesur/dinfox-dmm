/*
 * lvrm.c
 *
 *  Created on: 22 jan. 2023
 *      Author: Ludo
 */

#include "dinfox.h"
#include "lpuart.h"
#include "lvrm.h"
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
	LVRM_DATA_INDEX_VCOM_MV = DINFOX_DATA_INDEX_LAST,
	LVRM_DATA_INDEX_VOUT_MV,
	LVRM_DATA_INDEX_IOUT_UA,
	LVRM_DATA_INDEX_OUT_EN,
	LVRM_DATA_INDEX_LAST,
} LVRM_measurement_index_t;

/*** LVRM local macros ***/

static const char_t* LVRM_BOARD_NAME = "LVRM";
static const char_t* LVRM_DATA_NAME[LVRM_DATA_INDEX_LAST] = {DINFOX_COMMON_DATA_NAME, "VCOM =", "VOUT =", "IOUT =", "RELAY ="};
static const char_t* LVRM_DATA_UNIT[LVRM_DATA_INDEX_LAST] = {DINFOX_COMMON_DATA_UNIT, "mV", "mV", "uA", STRING_NULL};

/*** LVRM local structures ***/

typedef struct {
	RS485_address_t rs485_address;
	uint8_t read_index;
	char_t data_str[LVRM_DATA_INDEX_LAST][DINFOX_STRING_BUFFER_SIZE];
	uint8_t data_str_size[LVRM_DATA_INDEX_LAST];
	int32_t data_int[LVRM_REGISTER_LAST];
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
	string_status = STRING_append_string(lvrm_ctx.data_str[data_idx], DINFOX_STRING_BUFFER_SIZE, str, &(lvrm_ctx.data_str_size[data_idx])); \
	STRING_status_check(NODE_ERROR_BASE_STRING); \
}

/* GENERIC MACRO TO SET A DATA VALUE TO ERROR.
 * @param measurement_idx:	Measurement line index.
 * @return:					None.
 */
#define _LVRM_set_error() { \
	_LVRM_flush_data_str(data_idx); \
	_LVRM_append_string(DINFOX_DATA_ERROR); \
}

/* FLUSH ONE LINE OF THE MEASURERMENTS VALUE BUFFER.
 * @param:	None.
 * @return:	None.
 */
void _LVRM_flush_data_str(uint8_t measurement_index) {
	// Local variables.
	uint8_t idx = 0;
	// Char loop.
	for (idx=0 ; idx<DINFOX_STRING_BUFFER_SIZE ; idx++) lvrm_ctx.data_str[measurement_index][idx] = STRING_CHAR_NULL;
	lvrm_ctx.data_str_size[measurement_index] = 0;
}

/* FLUSH WHOLE DATAS VALUE BUFFER.
 * @param:	None.
 * @return:	None.
 */
void _LVRM_flush_measurements_value(void) {
	// Local variables.
	uint8_t idx = 0;
	// String data.
	for (idx=0 ; idx<LVRM_DATA_INDEX_LAST ; idx++) _LVRM_flush_data_str(idx);
	// Integer data.
	for (idx=0 ; idx<LVRM_REGISTER_LAST ; idx++) lvrm_ctx.data_int[idx] = 0;
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
 * @param:			None.
 * @return status:	Function execution status.
 */
NODE_status_t LVRM_perform_measurements(void) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	LPUART_status_t lpuart1_status = LPUART_SUCCESS;
	RS485_status_t rs485_status = RS485_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	RS485_reply_input_t reply_in;
	RS485_reply_output_t reply_out;
	uint8_t data_idx = 0;
	uint8_t register_address = 0;
	// Reset buffers.
	_LVRM_flush_measurements_value();
	// Turn RS485 on.
	lpuart1_status = LPUART1_power_on();
	LPUART1_status_check(NODE_ERROR_BASE_LPUART);
	// Common reply parameters.
	reply_in.type = RS485_REPLY_TYPE_VALUE;
	reply_in.timeout_ms = DINFOX_RS485_TIMEOUT_MS;
	reply_in.format = STRING_FORMAT_DECIMAL;
	// Common data loop.
	for (data_idx=0 ; data_idx<DINFOX_DATA_INDEX_LAST ; data_idx++) {
		status = DINFOX_read_data(lvrm_ctx.rs485_address, data_idx, lvrm_ctx.data_str[data_idx], &(lvrm_ctx.data_str_size[data_idx]), lvrm_ctx.data_int);
		if (status != NODE_SUCCESS) goto errors;
	}
	// Specific data loop.
	for (data_idx=DINFOX_DATA_INDEX_LAST ; data_idx<LVRM_DATA_INDEX_LAST ; data_idx++) {
		// Convert to register address.
		register_address = (data_idx + DINFOX_REGISTER_LAST - DINFOX_DATA_INDEX_LAST);
		// Read data.
		rs485_status = RS485_read_register(lvrm_ctx.rs485_address, register_address, &reply_in, &reply_out);
		RS485_status_check(NODE_ERROR_BASE_RS485);
		// Check reply.
		if (reply_out.status.all == 0) {
			// Update integer data.
			lvrm_ctx.data_int[register_address] = reply_out.value;
			// Specific print for relay.
			if (data_idx == LVRM_DATA_INDEX_OUT_EN) {
				if (lvrm_ctx.data_int[register_address] == 0) {
					_LVRM_append_string("OFF");
				}
				else {
					_LVRM_append_string("ON");
				}
			}
			else {
				_LVRM_append_string(reply_out.raw);
			}
			_LVRM_append_string((char_t*) LVRM_DATA_UNIT[data_idx]);
		}
		else {
			_LVRM_set_error();
		}
	}
errors:
	lvrm_ctx.read_index = 0;
	LPUART1_power_off();
	return status;
}

/* UNSTACK NODE DATA FORMATTED AS STRING.
 * @param measurement_name_ptr:		Pointer to string that will contain next measurement name.
 * @param measurement_value_ptr:	Pointer to string that will contain next measurement value.
 * @return status:					Function execution status.
 */
NODE_status_t LVRM_unstack_string_data(char_t** measurement_name_ptr, char_t** measurement_value_ptr) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	// Check index.
	if (lvrm_ctx.read_index < LVRM_DATA_INDEX_LAST) {
		// Update input pointers.
		(*measurement_name_ptr) = (char_t*) LVRM_DATA_NAME[lvrm_ctx.read_index];
		(*measurement_value_ptr) = (char_t*) lvrm_ctx.data_str[lvrm_ctx.read_index];
		// Increment index.
		lvrm_ctx.read_index++;
	}
	else {
		// Return NULL pointers.
		(*measurement_name_ptr) = NULL;
		(*measurement_value_ptr) = NULL;
		// Reset index.
		lvrm_ctx.read_index = 0;
	}
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

/* WRITE LVRM NODE DATA.
 * @param register_address:	Register to write.
 * @param value:			Value to write in register.
 * @return status:			Function execution status.
 */
NODE_status_t LVRM_write(uint8_t register_address, uint8_t value) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	return status;
}

