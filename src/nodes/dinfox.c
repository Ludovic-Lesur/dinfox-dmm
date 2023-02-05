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
#include "node_common.h"
#include "string.h"
#include "types.h"

/*** DINFOX local macros ***/

static const char_t* DINFOX_STRING_DATA_NAME[DINFOX_STRING_DATA_INDEX_LAST] = {"HW =", "SW =", "RESET =", "TMCU =", "VMCU ="};
static const char_t* DINFOX_STRING_DATA_UNIT[DINFOX_STRING_DATA_INDEX_LAST] = {STRING_NULL, STRING_NULL, STRING_NULL, "|C", "mV"};

/*** DINFOX functions ***/

/* UPDATE COMMON MEASUREMENTS OF DINFOX NODES.
 * @param rs485_address:		RS485 address.
 * @param string_data_index:	Data index to read.
 * @param name_ptr:				Pointer to the data name line.
 * @param value_ptr:			Pointer to the data value line.
 * @param int_ptr:				Pointer to the data integer value.
 * @return status:				Function execution status.
 */
NODE_status_t DINFOX_update_common_data(NODE_address_t rs485_address, uint8_t string_data_index, NODE_single_data_ptr_t* single_data_ptr) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	NODE_read_parameters_t read_input;
	NODE_reply_t reply;
	uint8_t error_flag = 0;
	uint8_t buffer_size = 0;
	// Common reply parameters.
#ifdef AM
	read_input.node_address = rs485_address;
#endif
	read_input.type = NODE_REPLY_TYPE_VALUE;
	read_input.timeout_ms = LBUS_TIMEOUT_MS;
	// Check parameters.
	if (single_data_ptr == NULL) {
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
		read_input.register_address = DINFOX_REGISTER_HW_VERSION_MAJOR;
		read_input.format = DINFOX_REGISTERS_FORMAT[read_input.register_address];
		status = LBUS_read_register(&read_input, &reply);
		if (status != NODE_SUCCESS) goto errors;
		// Check reply.
		if (reply.status.all == 0) {
			NODE_append_string_value(reply.raw);
			NODE_update_value(reply.value);
		}
		else {
			NODE_append_string_value(NODE_STRING_DATA_ERROR);
			break;
		}
		// Hardware version minor.
		read_input.register_address = DINFOX_REGISTER_HW_VERSION_MINOR;
		read_input.format = DINFOX_REGISTERS_FORMAT[read_input.register_address];
		status = LBUS_read_register(&read_input, &reply);
		if (status != NODE_SUCCESS) goto errors;
		// Check reply.
		if (reply.status.all == 0) {
			NODE_append_string_value(".");
			NODE_append_string_value(reply.raw);
			NODE_update_value(reply.value);
		}
		else {
			NODE_append_string_value(NODE_STRING_DATA_ERROR);
			break;
		}
		break;
	case DINFOX_STRING_DATA_INDEX_SW_VERSION:
		// Software version major.
		read_input.register_address = DINFOX_REGISTER_SW_VERSION_MAJOR;
		read_input.format = DINFOX_REGISTERS_FORMAT[read_input.register_address];
		status = LBUS_read_register(&read_input, &reply);
		if (status != NODE_SUCCESS) goto errors;
		// Check reply.
		if (reply.status.all == 0) {
			NODE_append_string_value(reply.raw);
			NODE_update_value(reply.value);
		}
		else {
			NODE_append_string_value(NODE_STRING_DATA_ERROR);
			break;
		}
		// Software version minor.
		read_input.register_address = DINFOX_REGISTER_SW_VERSION_MINOR;
		read_input.format = DINFOX_REGISTERS_FORMAT[read_input.register_address];
		status = LBUS_read_register(&read_input, &reply);
		if (status != NODE_SUCCESS) goto errors;
		// Check reply.
		if (reply.status.all == 0) {
			NODE_append_string_value(".");
			NODE_append_string_value(reply.raw);
			NODE_update_value(reply.value);
		}
		else {
			NODE_append_string_value(NODE_STRING_DATA_ERROR);
			break;
		}
		// Software version commit index.
		read_input.register_address = DINFOX_REGISTER_SW_VERSION_COMMIT_INDEX;
		read_input.format = DINFOX_REGISTERS_FORMAT[read_input.register_address];
		status = LBUS_read_register(&read_input, &reply);
		if (status != NODE_SUCCESS) goto errors;
		// Check reply.
		if (reply.status.all == 0) {
			NODE_append_string_value(".");
			NODE_append_string_value(reply.raw);
			NODE_update_value(reply.value);
		}
		else {
			NODE_append_string_value(NODE_STRING_DATA_ERROR);
			break;
		}
		// Software version commit ID.
		read_input.register_address = DINFOX_REGISTER_SW_VERSION_COMMIT_ID;
		read_input.format = DINFOX_REGISTERS_FORMAT[read_input.register_address];
		status = LBUS_read_register(&read_input, &reply);
		if (status != NODE_SUCCESS) goto errors;
		// Check reply.
		NODE_update_value((reply.status.all == 0) ? reply.value : 0);
		// Software version dirty flag.
		read_input.register_address = DINFOX_REGISTER_SW_VERSION_DIRTY_FLAG;
		read_input.format = DINFOX_REGISTERS_FORMAT[read_input.register_address];
		status = LBUS_read_register(&read_input, &reply);
		if (status != NODE_SUCCESS) goto errors;
		// Check reply.
		if (reply.status.all == 0) {
			// Check dirty flag.
			if (reply.value != 0) {
				NODE_append_string_value(".d");
				NODE_update_value(reply.value);
			}
		}
		else {
			NODE_append_string_value(NODE_STRING_DATA_ERROR);
		}
		break;
	case DINFOX_STRING_DATA_INDEX_RESET_FLAG:
		// Reset flags.
		read_input.register_address = DINFOX_REGISTER_RESET;
		read_input.format = DINFOX_REGISTERS_FORMAT[read_input.register_address];
		status = LBUS_read_register(&read_input, &reply);
		if (status != NODE_SUCCESS) goto errors;
		// Check reply.
		if (reply.status.all == 0) {
			NODE_append_string_value("0x");
			NODE_append_string_value(reply.raw);
			NODE_update_value(reply.value);
		}
		else {
			NODE_append_string_value(NODE_STRING_DATA_ERROR);
		}
		break;
	case DINFOX_STRING_DATA_INDEX_TMCU_DEGREES:
		// MCU temperature.
		read_input.register_address = DINFOX_REGISTER_TMCU_DEGREES;
		read_input.format = DINFOX_REGISTERS_FORMAT[read_input.register_address];
		status = LBUS_read_register(&read_input, &reply);
		if (status != NODE_SUCCESS) goto errors;
		// Check reply.
		if (reply.status.all == 0) {
			NODE_append_string_value(reply.raw);
			NODE_update_value(reply.value);
		}
		else {
			NODE_append_string_value(NODE_STRING_DATA_ERROR);
		}
		break;
	case DINFOX_STRING_DATA_INDEX_VMCU_MV:
		// MCU voltage.
		read_input.register_address = DINFOX_REGISTER_VMCU_MV;
		read_input.format = DINFOX_REGISTERS_FORMAT[read_input.register_address];
		status = LBUS_read_register(&read_input, &reply);
		if (status != NODE_SUCCESS) goto errors;
		// Check reply.
		if (reply.status.all == 0) {
			NODE_append_string_value(reply.raw);
			NODE_update_value(reply.value);
		}
		else {
			NODE_append_string_value(NODE_STRING_DATA_ERROR);
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
