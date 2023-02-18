/*
 * dinfox.c
 *
 *  Created on: 26 jan 2023
 *      Author: Ludo
 */

#include "dinfox.h"

#include "lbus.h"
#include "mode.h"
#include "node.h"
#include "string.h"
#include "types.h"

/*** DINFOX local macros ***/

static const char_t* DINFOX_STRING_DATA_NAME[DINFOX_STRING_DATA_INDEX_LAST] = {
	"HW =",
	"SW =",
	"RESET =",
	"TMCU =",
	"VMCU ="
};
static const char_t* DINFOX_STRING_DATA_UNIT[DINFOX_STRING_DATA_INDEX_LAST] = {
	STRING_NULL,
	STRING_NULL,
	STRING_NULL,
	"|C",
	"mV"
};
static const int32_t DINFOX_ERROR_VALUE[DINFOX_REGISTER_LAST] = {
	NODE_ERROR_VALUE_RS485_ADDRESS,
	NODE_ERROR_VALUE_BOARD_ID,
	NODE_ERROR_VALUE_VERSION,
	NODE_ERROR_VALUE_VERSION,
	NODE_ERROR_VALUE_VERSION,
	NODE_ERROR_VALUE_VERSION,
	NODE_ERROR_VALUE_COMMIT_INDEX,
	NODE_ERROR_VALUE_COMMIT_ID,
	NODE_ERROR_VALUE_DIRTY_FLAG,
	NODE_ERROR_VALUE_RESET_REASON,
	NODE_ERROR_VALUE_ERROR_STACK,
	NODE_ERROR_VALUE_TEMPERATURE,
	NODE_ERROR_VALUE_ANALOG_16BITS
};

/*** DINFOX functions ***/

/* UPDATE COMMON MEASUREMENTS OF DINFOX NODES.
 * @param rs485_address:		RS485 address.
 * @param string_data_index:	Data index to read.
 * @param single_string_data:	Pointer to the data string to be filled.
 * @param registers_value:		Registers value table.
 * @return status:				Function execution status.
 */
NODE_status_t DINFOX_update_data(NODE_address_t rs485_address, uint8_t string_data_index, NODE_single_string_data_t* single_string_data, int32_t* registers_value) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	NODE_read_parameters_t read_params;
	NODE_read_data_t read_data;
	NODE_access_status_t read_status;
	uint8_t error_flag = 0;
	uint8_t buffer_size = 0;
	// Common reply parameters.
#ifdef AM
	read_params.node_address = rs485_address;
#endif
	read_params.type = NODE_REPLY_TYPE_VALUE;
	read_params.timeout_ms = LBUS_TIMEOUT_MS;
	// Check parameters.
	if ((single_string_data == NULL) || (registers_value == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Check index.
	if (string_data_index >= DINFOX_STRING_DATA_INDEX_LAST) {
		status = NODE_ERROR_STRING_DATA_INDEX;
		goto errors;
	}
	// Add data name.
	NODE_append_string_name((char_t*) DINFOX_STRING_DATA_NAME[string_data_index]);
	buffer_size = 0;
	// Check index.
	switch (string_data_index) {
	case DINFOX_STRING_DATA_INDEX_HW_VERSION:
		// Hardware version major.
		read_params.register_address = DINFOX_REGISTER_HW_VERSION_MAJOR;
		read_params.format = DINFOX_REGISTERS_FORMAT[read_params.register_address];
		status = LBUS_read_register(&read_params, &read_data, &read_status);
		if (status != NODE_SUCCESS) goto errors;
		// Check reply.
		if (read_status.all == 0) {
			NODE_append_string_value(read_data.raw);
			NODE_update_value(read_params.register_address, read_data.value);
		}
		else {
			NODE_flush_string_value();
			NODE_append_string_value(NODE_ERROR_STRING);
			NODE_update_value(read_params.register_address, DINFOX_ERROR_VALUE[read_params.register_address]);
			break;
		}
		// Hardware version minor.
		read_params.register_address = DINFOX_REGISTER_HW_VERSION_MINOR;
		read_params.format = DINFOX_REGISTERS_FORMAT[read_params.register_address];
		status = LBUS_read_register(&read_params, &read_data, &read_status);
		if (status != NODE_SUCCESS) goto errors;
		// Check reply.
		if (read_status.all == 0) {
			NODE_append_string_value(".");
			NODE_append_string_value(read_data.raw);
			NODE_update_value(read_params.register_address, read_data.value);
		}
		else {
			NODE_flush_string_value();
			NODE_append_string_value(NODE_ERROR_STRING);
			NODE_update_value(read_params.register_address, DINFOX_ERROR_VALUE[read_params.register_address]);
			break;
		}
		break;
	case DINFOX_STRING_DATA_INDEX_SW_VERSION:
		// Software version major.
		read_params.register_address = DINFOX_REGISTER_SW_VERSION_MAJOR;
		read_params.format = DINFOX_REGISTERS_FORMAT[read_params.register_address];
		status = LBUS_read_register(&read_params, &read_data, &read_status);
		if (status != NODE_SUCCESS) goto errors;
		// Check reply.
		if (read_status.all == 0) {
			NODE_append_string_value(read_data.raw);
			NODE_update_value(read_params.register_address, read_data.value);
		}
		else {
			NODE_flush_string_value();
			NODE_append_string_value(NODE_ERROR_STRING);
			NODE_update_value(read_params.register_address, DINFOX_ERROR_VALUE[read_params.register_address]);
			break;
		}
		// Software version minor.
		read_params.register_address = DINFOX_REGISTER_SW_VERSION_MINOR;
		read_params.format = DINFOX_REGISTERS_FORMAT[read_params.register_address];
		status = LBUS_read_register(&read_params, &read_data, &read_status);
		if (status != NODE_SUCCESS) goto errors;
		// Check reply.
		if (read_status.all == 0) {
			NODE_append_string_value(".");
			NODE_append_string_value(read_data.raw);
			NODE_update_value(read_params.register_address, read_data.value);
		}
		else {
			NODE_flush_string_value();
			NODE_append_string_value(NODE_ERROR_STRING);
			NODE_update_value(read_params.register_address, DINFOX_ERROR_VALUE[read_params.register_address]);
			break;
		}
		// Software version commit index.
		read_params.register_address = DINFOX_REGISTER_SW_VERSION_COMMIT_INDEX;
		read_params.format = DINFOX_REGISTERS_FORMAT[read_params.register_address];
		status = LBUS_read_register(&read_params, &read_data, &read_status);
		if (status != NODE_SUCCESS) goto errors;
		// Check reply.
		if (read_status.all == 0) {
			NODE_append_string_value(".");
			NODE_append_string_value(read_data.raw);
			NODE_update_value(read_params.register_address, read_data.value);
		}
		else {
			NODE_flush_string_value();
			NODE_append_string_value(NODE_ERROR_STRING);
			NODE_update_value(read_params.register_address, DINFOX_ERROR_VALUE[read_params.register_address]);
			break;
		}
		// Software version commit ID.
		read_params.register_address = DINFOX_REGISTER_SW_VERSION_COMMIT_ID;
		read_params.format = DINFOX_REGISTERS_FORMAT[read_params.register_address];
		status = LBUS_read_register(&read_params, &read_data, &read_status);
		if (status != NODE_SUCCESS) goto errors;
		// Check reply.
		if (read_status.all == 0) {
			NODE_update_value(read_params.register_address, read_data.value);
		}
		else {
			NODE_update_value(read_params.register_address, DINFOX_ERROR_VALUE[read_params.register_address]);
		}
		// Software version dirty flag.
		read_params.register_address = DINFOX_REGISTER_SW_VERSION_DIRTY_FLAG;
		read_params.format = DINFOX_REGISTERS_FORMAT[read_params.register_address];
		status = LBUS_read_register(&read_params, &read_data, &read_status);
		if (status != NODE_SUCCESS) goto errors;
		// Check reply.
		if (read_status.all == 0) {
			// Check dirty flag.
			if (read_data.value != 0) {
				NODE_append_string_value(".d");
				NODE_update_value(read_params.register_address, read_data.value);
			}
		}
		else {
			NODE_flush_string_value();
			NODE_append_string_value(NODE_ERROR_STRING);
			NODE_update_value(read_params.register_address, DINFOX_ERROR_VALUE[read_params.register_address]);
		}
		break;
	case DINFOX_STRING_DATA_INDEX_RESET_REASON:
		// Reset flags.
		read_params.register_address = DINFOX_REGISTER_RESET_REASON;
		read_params.format = DINFOX_REGISTERS_FORMAT[read_params.register_address];
		status = LBUS_read_register(&read_params, &read_data, &read_status);
		if (status != NODE_SUCCESS) goto errors;
		// Check reply.
		if (read_status.all == 0) {
			NODE_append_string_value("0x");
			NODE_append_string_value(read_data.raw);
			NODE_update_value(read_params.register_address, read_data.value);
		}
		else {
			NODE_flush_string_value();
			NODE_append_string_value(NODE_ERROR_STRING);
			NODE_update_value(read_params.register_address, DINFOX_ERROR_VALUE[read_params.register_address]);
		}
		break;
	case DINFOX_STRING_DATA_INDEX_TMCU_DEGREES:
		// MCU temperature.
		read_params.register_address = DINFOX_REGISTER_TMCU_DEGREES;
		read_params.format = DINFOX_REGISTERS_FORMAT[read_params.register_address];
		status = LBUS_read_register(&read_params, &read_data, &read_status);
		if (status != NODE_SUCCESS) goto errors;
		// Check reply.
		if (read_status.all == 0) {
			NODE_append_string_value(read_data.raw);
			NODE_update_value(read_params.register_address, read_data.value);
		}
		else {
			NODE_flush_string_value();
			NODE_append_string_value(NODE_ERROR_STRING);
			NODE_update_value(read_params.register_address, DINFOX_ERROR_VALUE[read_params.register_address]);
		}
		break;
	case DINFOX_STRING_DATA_INDEX_VMCU_MV:
		// MCU voltage.
		read_params.register_address = DINFOX_REGISTER_VMCU_MV;
		read_params.format = DINFOX_REGISTERS_FORMAT[read_params.register_address];
		status = LBUS_read_register(&read_params, &read_data, &read_status);
		if (status != NODE_SUCCESS) goto errors;
		// Check reply.
		if (read_status.all == 0) {
			NODE_append_string_value(read_data.raw);
			NODE_update_value(read_params.register_address, read_data.value);
		}
		else {
			NODE_flush_string_value();
			NODE_append_string_value(NODE_ERROR_STRING);
			NODE_update_value(read_params.register_address, DINFOX_ERROR_VALUE[read_params.register_address]);
		}
		break;
	default:
		status = NODE_ERROR_STRING_DATA_INDEX;
		goto errors;
	}
	// Add unit if no error.
	if (error_flag == 0) {
		NODE_append_string_value((char_t*) DINFOX_STRING_DATA_UNIT[string_data_index]);
	}
errors:
	return status;
}
