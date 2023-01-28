/*
 * dinfox.c
 *
 *  Created on: 26 jan 2023
 *      Author: Ludo
 */

#include "dinfox.h"

#include "node_common.h"
#include "rs485.h"
#include "rs485_common.h"
#include "string.h"
#include "types.h"

/*** DINFOX local macros ***/

static const char_t* DINFOX_DATA_UNIT[DINFOX_DATA_INDEX_LAST] = {DINFOX_COMMON_DATA_UNIT};

/*** DINFOX local functions ***/

/* FLUSH CURRENT DATA LINE.
 * @param:	None.
 * @return:	None.
 */
#define _DINFOX_flush_measurement_value(void) { \
	uint8_t idx = 0; \
	for (idx=0 ; idx<DINFOX_STRING_BUFFER_SIZE ; idx++) data_str_ptr[idx] = STRING_CHAR_NULL; \
	(*data_str_size) = 0; \
}

/* GENERIC MACRO TO APPEND A STRING TO THE DATA VALUE BUFFER.
 * @param measurement_idx:	Measurement line index.
 * @param str:				String to append.
 * @return:					None.
 */
#define _DINFOX_append_string(str) { \
	string_status = STRING_append_string(data_str_ptr, DINFOX_STRING_BUFFER_SIZE, str, data_str_size); \
	STRING_status_check(NODE_ERROR_BASE_STRING); \
}

/* GENERIC MACRO TO SET A DATA VALUE TO ERROR.
 * @param measurement_idx:	Measurement line index.
 * @return:					None.
 */
#define _DINFOX_set_error() { \
	_DINFOX_flush_measurement_value(); \
	_DINFOX_append_string(DINFOX_DATA_ERROR); \
	error_flag = 1; \
}

/*** DINFOX functions ***/

/* UNSTACK COMMON MEASUREMENTS OF DINFOX NODES.
 * @param rs485_address:	RS485 address.
 * @param data_index:		Data index to read.
 * @param data_value_ptr:	Pointer to the data value line.
 * @param data_value_size:	Pointer that will contain data value line size.
 * @return status:			Function execution status.
 */
NODE_status_t DINFOX_read_data(RS485_address_t rs485_address, DINFOX_common_data_index_t data_index, char_t* data_str_ptr, uint8_t* data_str_size, int32_t* data_int_ptr) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	RS485_status_t rs485_status = RS485_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	RS485_reply_input_t reply_in;
	RS485_reply_output_t reply_out;
	uint8_t error_flag = 0;
	// Common reply parameters.
	reply_in.type = RS485_REPLY_TYPE_VALUE;
	reply_in.timeout_ms = DINFOX_RS485_TIMEOUT_MS;
	// Check parameters.
	if ((data_str_ptr == NULL) || (data_str_size == NULL) || (data_int_ptr == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Check index.
	switch (data_index) {
	case DINFOX_DATA_INDEX_HW_VERSION:
		// Hardware version major.
		reply_in.format = STRING_FORMAT_DECIMAL;
		rs485_status = RS485_read_register(rs485_address, DINFOX_REGISTER_HW_VERSION_MAJOR, &reply_in, &reply_out);
		RS485_status_check(NODE_ERROR_BASE_RS485);
		// Check reply.
		if (reply_out.status.all == 0) {
			_DINFOX_append_string(reply_out.raw);
			data_int_ptr[DINFOX_REGISTER_HW_VERSION_MAJOR] = reply_out.value;
		}
		else {
			_DINFOX_set_error();
			break;
		}
		// Hardware version minor.
		rs485_status = RS485_read_register(rs485_address, DINFOX_REGISTER_HW_VERSION_MINOR, &reply_in, &reply_out);
		RS485_status_check(NODE_ERROR_BASE_RS485);
		// Check reply.
		if (reply_out.status.all == 0) {
			_DINFOX_append_string(".");
			_DINFOX_append_string(reply_out.raw);
			data_int_ptr[DINFOX_REGISTER_HW_VERSION_MINOR] = reply_out.value;
		}
		else {
			_DINFOX_set_error();
			break;
		}
		break;
	case DINFOX_DATA_INDEX_SW_VERSION:
		// Software version major.
		reply_in.format = STRING_FORMAT_DECIMAL;
		rs485_status = RS485_read_register(rs485_address, DINFOX_REGISTER_SW_VERSION_MAJOR, &reply_in, &reply_out);
		RS485_status_check(NODE_ERROR_BASE_RS485);
		// Check reply.
		if (reply_out.status.all == 0) {
			_DINFOX_append_string(reply_out.raw);
			data_int_ptr[DINFOX_REGISTER_SW_VERSION_MAJOR] = reply_out.value;
		}
		else {
			_DINFOX_set_error();
			break;
		}
		// Software version minor.
		rs485_status = RS485_read_register(rs485_address, DINFOX_REGISTER_SW_VERSION_MINOR, &reply_in, &reply_out);
		RS485_status_check(NODE_ERROR_BASE_RS485);
		// Check reply.
		if (reply_out.status.all == 0) {
			_DINFOX_append_string(".");
			_DINFOX_append_string(reply_out.raw);
			data_int_ptr[DINFOX_REGISTER_SW_VERSION_MINOR] = reply_out.value;
		}
		else {
			_DINFOX_set_error();
			break;
		}
		// Software version commit index.
		rs485_status = RS485_read_register(rs485_address, DINFOX_REGISTER_SW_VERSION_COMMIT_INDEX, &reply_in, &reply_out);
		RS485_status_check(NODE_ERROR_BASE_RS485);
		// Check reply.
		if (reply_out.status.all == 0) {
			_DINFOX_append_string(".");
			_DINFOX_append_string(reply_out.raw);
			data_int_ptr[DINFOX_REGISTER_SW_VERSION_COMMIT_INDEX] = reply_out.value;
		}
		else {
			_DINFOX_set_error();
			break;
		}
		// Software version commit ID.
		reply_in.format = STRING_FORMAT_HEXADECIMAL;
		rs485_status = RS485_read_register(rs485_address, DINFOX_REGISTER_SW_VERSION_COMMIT_ID, &reply_in, &reply_out);
		RS485_status_check(NODE_ERROR_BASE_RS485);
		// Check reply.
		data_int_ptr[DINFOX_REGISTER_SW_VERSION_COMMIT_ID] = (reply_out.status.all == 0) ? reply_out.value : 0;
		// Software version dirty flag.
		reply_in.format = STRING_FORMAT_BOOLEAN;
		rs485_status = RS485_read_register(rs485_address, DINFOX_REGISTER_SW_VERSION_DIRTY_FLAG, &reply_in, &reply_out);
		RS485_status_check(NODE_ERROR_BASE_RS485);
		// Check reply.
		if (reply_out.status.all == 0) {
			// Check dirty flag.
			if (reply_out.value != 0) {
				_DINFOX_append_string(".d");
				data_int_ptr[DINFOX_REGISTER_SW_VERSION_DIRTY_FLAG] = reply_out.value;
			}
		}
		else {
			_DINFOX_set_error();
		}
		break;
	case DINFOX_DATA_INDEX_RESET_FLAG:
		// Reset flags.
		reply_in.format = STRING_FORMAT_HEXADECIMAL;
		rs485_status = RS485_read_register(rs485_address, DINFOX_REGISTER_RESET, &reply_in, &reply_out);
		RS485_status_check(NODE_ERROR_BASE_RS485);
		// Check reply.
		if (reply_out.status.all == 0) {
			_DINFOX_append_string("0x");
			_DINFOX_append_string(reply_out.raw);
			data_int_ptr[DINFOX_REGISTER_RESET] = reply_out.value;
		}
		else {
			_DINFOX_set_error();
		}
		break;
	case DINFOX_DATA_INDEX_TMCU_DEGREES:
		// MCU temperature.
		reply_in.format = STRING_FORMAT_DECIMAL;
		rs485_status = RS485_read_register(rs485_address, DINFOX_REGISTER_TMCU_DEGREES, &reply_in, &reply_out);
		RS485_status_check(NODE_ERROR_BASE_RS485);
		// Check reply.
		if (reply_out.status.all == 0) {
			_DINFOX_append_string(reply_out.raw);
			data_int_ptr[DINFOX_REGISTER_TMCU_DEGREES] = reply_out.value;
		}
		else {
			_DINFOX_set_error();
		}
		break;
	case DINFOX_DATA_INDEX_VMCU_MV:
		// MCU voltage.
		reply_in.format = STRING_FORMAT_DECIMAL;
		rs485_status = RS485_read_register(rs485_address, DINFOX_REGISTER_VMCU_MV, &reply_in, &reply_out);
		RS485_status_check(NODE_ERROR_BASE_RS485);
		// Check reply.
		if (reply_out.status.all == 0) {
			_DINFOX_append_string(reply_out.raw);
			data_int_ptr[DINFOX_REGISTER_VMCU_MV] = reply_out.value;
		}
		else {
			_DINFOX_set_error();
		}
		break;
	default:
		status = NODE_ERROR_DATA_INDEX;
		goto errors;
	}
	// Add unit if no error.
	if (error_flag == 0) {
		_DINFOX_append_string((char_t*) DINFOX_DATA_UNIT[data_index]);
	}
errors:
	return status;
}
