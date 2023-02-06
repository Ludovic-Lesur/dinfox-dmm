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
NODE_status_t LVRM_update_data(NODE_address_t rs485_address, uint8_t string_data_index, NODE_single_data_ptr_t* single_data_ptr) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	NODE_read_parameters_t read_params;
	NODE_read_data_t read_data;
	NODE_access_status_t read_status;
	uint8_t register_address = 0;
	uint8_t buffer_size = 0;
	// Check index.
	if ((string_data_index < DINFOX_STRING_DATA_INDEX_LAST) || (string_data_index >= LVRM_STRING_DATA_INDEX_LAST)) {
		status = NODE_ERROR_STRING_DATA_INDEX;
		goto errors;
	}
	// Common read input parameters.
#ifdef AM
	read_params.node_address = rs485_address;
#endif
	read_params.type = NODE_READ_TYPE_VALUE;
	read_params.timeout_ms = LBUS_TIMEOUT_MS;
	read_params.format = STRING_FORMAT_DECIMAL;
	// Convert to register address.
	register_address = (string_data_index + DINFOX_REGISTER_LAST - DINFOX_STRING_DATA_INDEX_LAST);
	read_params.register_address = register_address;
	// Read data.
	status = LBUS_read_register(&read_params, &read_data, &read_status);
	if (status != NODE_SUCCESS) goto errors;
	// Add data name.
	NODE_append_string_name((char_t*) LVRM_STRING_DATA_NAME[string_data_index - DINFOX_STRING_DATA_INDEX_LAST]);
	buffer_size = 0;
	// Add data value.
	if (read_status.all == 0) {
		// Update integer data.
		NODE_update_value(read_data.value);
		// Specific print for relay.
		if (string_data_index == LVRM_STRING_DATA_INDEX_OUT_EN) {
			NODE_append_string_value((read_data.value == 0) ? "OFF" : "ON");
		}
		else {
			NODE_append_string_value(read_data.raw);
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

/* GET LVRM NODE SIGFOX PAYLOAD.
 * @param payload_type:		Sigfox payload type.
 * @param ul_payload:		Pointer that will contain UL payload of the node.
 * @param ul_payload_size:	Pointer to byte that will contain UL payload size.
 * @return status:			Function execution status.
 */
NODE_status_t LVRM_get_sigfox_payload(NODE_sigfox_payload_type_t payload_type, uint8_t* ul_payload, uint8_t* ul_payload_size) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	return status;
}
