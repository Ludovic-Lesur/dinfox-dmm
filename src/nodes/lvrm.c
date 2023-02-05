/*
 * lvrm.c
 *
 *  Created on: 22 jan. 2023
 *      Author: Ludo
 */

#include "lvrm.h"

#include "dinfox.h"
#include "lbus.h"
#include "mode.h"
#include "node_common.h"
#include "string.h"

/*** LVRM local macros ***/

static const char_t* LVRM_STRING_DATA_NAME[LVRM_NUMBER_OF_SPECIFIC_STRING_DATA] = {"VCOM =", "VOUT =", "IOUT =", "RELAY ="};
static const char_t* LVRM_STRING_DATA_UNIT[LVRM_NUMBER_OF_SPECIFIC_STRING_DATA] = {"mV", "mV", "uA", STRING_NULL};

/*** LVRM functions ***/

/* RETRIEVE SPECIFIC DATA OF LVRM NODE.
 * @param rs485_address:		RS485 address of the node to update.
 * @param string_data_index:	Node string data index.
 * @param single_data_ptr:		Pointer to the data string and value to fill.
 * @return status:				Function execution status.
 */
NODE_status_t LVRM_update_specific_data(NODE_address_t rs485_address, uint8_t string_data_index, NODE_single_data_ptr_t* single_data_ptr) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	LBUS_status_t lbus_status = LBUS_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	NODE_read_parameters_t read_input;
	NODE_reply_t reply;
	uint8_t register_address = 0;
	uint8_t buffer_size = 0;
	// Common read input parameters.
#ifdef AM
	read_input.node_address = rs485_address;
#endif
	read_input.type = NODE_REPLY_TYPE_VALUE;
	read_input.timeout_ms = LBUS_TIMEOUT_MS;
	read_input.format = STRING_FORMAT_DECIMAL;
	// Check index.
	if ((string_data_index < DINFOX_STRING_DATA_INDEX_LAST) || (string_data_index >= LVRM_STRING_DATA_INDEX_LAST)) {
		status = NODE_ERROR_STRING_DATA_INDEX;
		goto errors;
	}
	// Convert to register address.
	register_address = (string_data_index + DINFOX_REGISTER_LAST - DINFOX_STRING_DATA_INDEX_LAST);
	read_input.register_address = register_address;
	// Read data.
	lbus_status = LBUS_read_register(&read_input, &reply);
	LBUS_status_check(NODE_ERROR_BASE_LBUS);
	// Add data name.
	NODE_append_string_name((char_t*) LVRM_STRING_DATA_NAME[string_data_index - DINFOX_STRING_DATA_INDEX_LAST]);
	buffer_size = 0;
	// Add data value.
	if (reply.status.all == 0) {
		// Update integer data.
		NODE_update_value(reply.value);
		// Specific print for relay.
		if (string_data_index == LVRM_STRING_DATA_INDEX_OUT_EN) {
			NODE_append_string_value((reply.value == 0) ? "OFF" : "ON");
		}
		else {
			NODE_append_string_value(reply.raw);
		}
		// Add unit.
		NODE_append_string_value((char_t*) LVRM_STRING_DATA_UNIT[string_data_index - DINFOX_STRING_DATA_INDEX_LAST]);
	}
	else {
		NODE_append_string_value(NODE_STRING_DATA_ERROR);
	}
errors:
	return status;
}

/* GET NODE SIGFOX PAYLOAD.
 * @param ul_payload:		Pointer that will contain UL payload of the node.
 * @param ul_payload_size:	Pointer to byte that will contain UL payload size.
 * @return status:			Function execution status.
 */
NODE_status_t LVRM_get_sigfox_payload(NODE_sigfox_payload_type_t sigfox_payload_type, uint8_t* ul_payload, uint8_t* ul_payload_size) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	return status;
}
