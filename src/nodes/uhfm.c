/*
 * uhfm.c
 *
 *  Created on: 14 feb. 2023
 *      Author: Ludo
 */

#include "uhfm.h"

#include "dinfox.h"
#include "lbus.h"
#include "mode.h"
#include "node_common.h"
#include "string.h"

/*** UHFM local macros ***/

#define UHFM_SIGFOX_PAYLOAD_MONITORING_SIZE		5
#define UHFM_COMMAND_BUFFER_SIZE_BYTES			64

#define UHFM_COMMAND_SEND						"RS$SF="

static const char_t* UHFM_STRING_DATA_NAME[UHFM_NUMBER_OF_SPECIFIC_STRING_DATA] = {
	"VRF ="
};
static const char_t* UHFM_STRING_DATA_UNIT[UHFM_NUMBER_OF_SPECIFIC_STRING_DATA] = {
	"mV"
};
static const int32_t UHFM_ERROR_VALUE[UHFM_NUMBER_OF_SPECIFIC_REGISTERS] = {
	NODE_ERROR_VALUE_ANALOG_16BITS,
};

/*** UHFM local structures ***/

typedef union {
	uint8_t frame[UHFM_SIGFOX_PAYLOAD_MONITORING_SIZE];
	struct {
		unsigned vmcu_mv : 16;
		unsigned tmcu_degrees : 8;
		unsigned vrf_mv : 16;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} UHFM_sigfox_payload_monitoring_t;

/*** UHFM functions ***/

/* RETRIEVE SPECIFIC DATA OF UHFM NODE.
 * @param rs485_address:		RS485 address of the node to update.
 * @param string_data_index:	Node string data index.
 * @param single_string_data:	Pointer to the data string to be filled.
 * @param registers_value:		Registers value table.
 * @return status:				Function execution status.
 */
NODE_status_t UHFM_update_data(NODE_address_t rs485_address, uint8_t string_data_index, NODE_single_string_data_t* single_string_data, int32_t* registers_value) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	NODE_read_parameters_t read_params;
	NODE_read_data_t read_data;
	NODE_access_status_t read_status;
	uint8_t register_address = 0;
	uint8_t buffer_size = 0;
	// Check index.
	if ((string_data_index < DINFOX_STRING_DATA_INDEX_LAST) || (string_data_index >= UHFM_STRING_DATA_INDEX_LAST)) {
		status = NODE_ERROR_STRING_DATA_INDEX;
		goto errors;
	}
	// Convert to register address.
	register_address = (string_data_index + DINFOX_REGISTER_LAST - DINFOX_STRING_DATA_INDEX_LAST);
	// Read parameters.
#ifdef AM
	read_params.node_address = rs485_address;
#endif
	read_params.register_address = register_address;
	read_params.type = NODE_REPLY_TYPE_VALUE;
	read_params.timeout_ms = LBUS_TIMEOUT_MS;
	read_params.format = STRING_FORMAT_DECIMAL;
	// Read data.
	status = LBUS_read_register(&read_params, &read_data, &read_status);
	if (status != NODE_SUCCESS) goto errors;
	// Add data name.
	NODE_append_string_name((char_t*) UHFM_STRING_DATA_NAME[string_data_index - DINFOX_STRING_DATA_INDEX_LAST]);
	buffer_size = 0;
	// Add data value.
	if (read_status.all == 0) {
		// Add value.
		NODE_append_string_value(read_data.raw);
		// Add unit.
		NODE_append_string_value((char_t*) UHFM_STRING_DATA_UNIT[string_data_index - DINFOX_STRING_DATA_INDEX_LAST]);
		// Update integer data.
		NODE_update_value(register_address, read_data.value);
	}
	else {
		NODE_flush_string_value();
		NODE_append_string_value(NODE_ERROR_STRING);
		NODE_update_value(register_address, UHFM_ERROR_VALUE[register_address]);
	}
errors:
	return status;
}

/* GET UHFM NODE SIGFOX PAYLOAD.
 * @param integer_data_value:	Pointer to the node registers value.
 * @param sigfox_payload_type:	Sigfox payload type.
 * @param sigfox_payload:		Pointer that will contain the specific sigfox payload of the node.
 * @param sigfox_payload_size:	Pointer to byte that will contain sigfox payload size.
 * @return status:				Function execution status.
 */
NODE_status_t UHFM_get_sigfox_payload(int32_t* integer_data_value, NODE_sigfox_payload_type_t sigfox_payload_type, uint8_t* sigfox_payload, uint8_t* sigfox_payload_size) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	UHFM_sigfox_payload_monitoring_t sigfox_payload_monitoring;
	uint8_t idx = 0;
	// Check parameters.
	if ((integer_data_value == NULL) || (sigfox_payload == NULL) || (sigfox_payload_size == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Check type.
	switch (sigfox_payload_type) {
	case NODE_SIGFOX_PAYLOAD_TYPE_MONITORING:
		// Build monitoring payload.
		sigfox_payload_monitoring.vmcu_mv = integer_data_value[DINFOX_REGISTER_VMCU_MV];
		sigfox_payload_monitoring.tmcu_degrees = integer_data_value[DINFOX_REGISTER_TMCU_DEGREES];
		sigfox_payload_monitoring.vrf_mv = integer_data_value[UHFM_REGISTER_VRF_MV];
		// Copy payload.
		for (idx=0 ; idx<UHFM_SIGFOX_PAYLOAD_MONITORING_SIZE ; idx++) {
			sigfox_payload[idx] = sigfox_payload_monitoring.frame[idx];
		}
		(*sigfox_payload_size) = UHFM_SIGFOX_PAYLOAD_MONITORING_SIZE;
		break;
	case NODE_SIGFOX_PAYLOAD_TYPE_DATA:
		// None data frame.
		(*sigfox_payload_size) = 0;
		break;
	default:
		status = NODE_ERROR_SIGFOX_PAYLOAD_TYPE;
		goto errors;
	}
errors:
	return status;
}

/* SEND SIGFOX MESSAGE WITH UHFM MODULE.
 * @param rs485_address:	RS485 address of the UHFM node to use.
 * @param sigfox_message:	Pointer to the Sigfox message structure.
 * @param send_status:		Pointer to the sending status.
 * @return status:			Function execution status.
 */
NODE_status_t UHFM_send_sigfox_message(NODE_address_t rs485_address, UHFM_sigfox_message_t* sigfox_message, NODE_access_status_t* send_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	NODE_command_parameters_t command_params;
	NODE_reply_parameters_t reply_params;
	NODE_read_data_t unused_read_data;
	char_t command[UHFM_COMMAND_BUFFER_SIZE_BYTES] = {STRING_CHAR_NULL};
	uint8_t command_size = 0;
	uint8_t idx = 0;
	uint32_t timeout_ms = 0;
	// Build command.
	string_status = STRING_append_string(command, UHFM_COMMAND_BUFFER_SIZE_BYTES, UHFM_COMMAND_SEND, &command_size);
	STRING_status_check(NODE_ERROR_BASE_STRING);
	// UL payload bytes loop.
	for (idx=0 ; idx<(sigfox_message -> ul_payload_size) ; idx++) {
		string_status = STRING_append_value(command, UHFM_COMMAND_BUFFER_SIZE_BYTES, (sigfox_message -> ul_payload)[idx], STRING_FORMAT_HEXADECIMAL, 0, &command_size);
		STRING_status_check(NODE_ERROR_BASE_STRING);
	}
	// Check bidirectional flag.
	if ((sigfox_message -> bidirectional_flag) != 0) {
		// Append parameter.
		string_status = STRING_append_string(command, UHFM_COMMAND_BUFFER_SIZE_BYTES, ",1", &command_size);
		STRING_status_check(NODE_ERROR_BASE_STRING);
		// Update timeout.
		timeout_ms = 60000;
	}
	else {
		timeout_ms = 10000;
	}
	// Build command structure.
#ifdef AM
	command_params.node_address = rs485_address;
#endif
	command_params.command = (char_t*) command;
	// Build reply structure.
	reply_params.type = NODE_REPLY_TYPE_OK;
	reply_params.format = STRING_FORMAT_BOOLEAN;
	reply_params.timeout_ms = timeout_ms;
	// Send command.
	status = LBUS_send_command(&command_params, &reply_params, &unused_read_data, send_status);
errors:
	return status;
}
