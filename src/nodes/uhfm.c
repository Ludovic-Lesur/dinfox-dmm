/*
 * uhfm.c
 *
 *  Created on: 14 feb. 2023
 *      Author: Ludo
 */

#include "uhfm.h"

#include "at_bus.h"
#include "dinfox.h"
#include "mode.h"
#include "string.h"

/*** UHFM local macros ***/

#define UHFM_SIGFOX_PAYLOAD_MONITORING_SIZE		5
#define UHFM_COMMAND_BUFFER_SIZE_BYTES			64

#define UHFM_COMMAND_SEND						"AT$SF="
#define UHFM_COMMAND_READ_DL_PAYLOAD			"AT$DL?"

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
 * @param data_update:	Pointer to the data update structure.
 * @return status:		Function execution status.
 */
NODE_status_t UHFM_update_data(NODE_data_update_t* data_update) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	NODE_read_parameters_t read_params;
	NODE_read_data_t read_data;
	NODE_access_status_t read_status;
	uint8_t register_address = 0;
	uint8_t buffer_size = 0;
	// Check parameters.
	if (data_update == NULL) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	if (((data_update -> name_ptr) == NULL) || ((data_update -> value_ptr) == NULL) || ((data_update -> registers_value_ptr) == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Check index.
	if (((data_update -> string_data_index) < DINFOX_STRING_DATA_INDEX_LAST) || ((data_update -> string_data_index) >= UHFM_STRING_DATA_INDEX_LAST)) {
		status = NODE_ERROR_STRING_DATA_INDEX;
		goto errors;
	}
	// Convert to register address.
	register_address = ((data_update -> string_data_index) + DINFOX_REGISTER_LAST - DINFOX_STRING_DATA_INDEX_LAST);
	// Read parameters.
#ifdef AM
	read_params.node_address = (data_update -> node_address);
#endif
	read_params.register_address = register_address;
	read_params.type = NODE_REPLY_TYPE_VALUE;
	read_params.timeout_ms = AT_BUS_DEFAULT_TIMEOUT_MS;
	read_params.format = STRING_FORMAT_DECIMAL;
	// Configure read data.
	read_data.raw = NULL;
	read_data.value = 0;
	read_data.byte_array = NULL;
	read_data.extracted_length = 0;
	// Read data.
	status = AT_BUS_read_register(&read_params, &read_data, &read_status);
	if (status != NODE_SUCCESS) goto errors;
	// Add data name.
	NODE_append_string_name((char_t*) UHFM_STRING_DATA_NAME[(data_update -> string_data_index) - DINFOX_STRING_DATA_INDEX_LAST]);
	buffer_size = 0;
	// Add data value.
	if (read_status.all == 0) {
		// Add value.
		NODE_append_string_value(read_data.raw);
		// Add unit.
		NODE_append_string_value((char_t*) UHFM_STRING_DATA_UNIT[(data_update -> string_data_index) - DINFOX_STRING_DATA_INDEX_LAST]);
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
 * @param ul_payload_type:		Sigfox payload type.
 * @param ul_payload:			Pointer that will contain the specific sigfox payload of the node.
 * @param ul_payload_size:		Pointer to byte that will contain sigfox payload size.
 * @return status:				Function execution status.
 */
NODE_status_t UHFM_get_sigfox_ul_payload(int32_t* integer_data_value, NODE_sigfox_ul_payload_type_t ul_payload_type, uint8_t* ul_payload, uint8_t* ul_payload_size) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	UHFM_sigfox_payload_monitoring_t sigfox_payload_monitoring;
	uint8_t idx = 0;
	// Check parameters.
	if ((integer_data_value == NULL) || (ul_payload == NULL) || (ul_payload_size == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Check type.
	switch (ul_payload_type) {
	case NODE_SIGFOX_PAYLOAD_TYPE_MONITORING:
		// Build monitoring payload.
		sigfox_payload_monitoring.vmcu_mv = integer_data_value[DINFOX_REGISTER_VMCU_MV];
		sigfox_payload_monitoring.tmcu_degrees = integer_data_value[DINFOX_REGISTER_TMCU_DEGREES];
		sigfox_payload_monitoring.vrf_mv = integer_data_value[UHFM_REGISTER_VRF_MV];
		// Copy payload.
		for (idx=0 ; idx<UHFM_SIGFOX_PAYLOAD_MONITORING_SIZE ; idx++) {
			ul_payload[idx] = sigfox_payload_monitoring.frame[idx];
		}
		(*ul_payload_size) = UHFM_SIGFOX_PAYLOAD_MONITORING_SIZE;
		break;
	case NODE_SIGFOX_PAYLOAD_TYPE_DATA:
		// None data frame.
		(*ul_payload_size) = 0;
		break;
	default:
		status = NODE_ERROR_SIGFOX_PAYLOAD_TYPE;
		goto errors;
	}
errors:
	return status;
}

#ifdef AM
/* SEND SIGFOX MESSAGE WITH UHFM MODULE.
 * @param node_address:		Address of the UHFM node to use.
 * @param sigfox_message:	Pointer to the Sigfox message structure.
 * @param send_status:		Pointer to the sending status.
 * @return status:			Function execution status.
 */
NODE_status_t UHFM_send_sigfox_message(NODE_address_t node_address, UHFM_sigfox_message_t* sigfox_message, NODE_access_status_t* send_status) {
#else
/* SEND SIGFOX MESSAGE WITH UHFM MODULE.
 * @param sigfox_message:	Pointer to the Sigfox message structure.
 * @param send_status:		Pointer to the sending status.
 * @return status:			Function execution status.
 */
NODE_status_t UHFM_send_sigfox_message(UHFM_sigfox_message_t* sigfox_message, NODE_access_status_t* send_status) {
#endif
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	NODE_command_parameters_t command_params;
	NODE_reply_parameters_t reply_params;
	NODE_read_data_t read_data;
	char_t command[UHFM_COMMAND_BUFFER_SIZE_BYTES] = {STRING_CHAR_NULL};
	uint8_t command_size = 0;
	uint8_t idx = 0;
	uint32_t timeout_ms = 0;
	// Set command parameters.
#ifdef AM
	command_params.node_address = node_address;
#endif
	command_params.command = (char_t*) command;
	// Configure read data.
	read_data.raw = NULL;
	read_data.value = 0;
	read_data.byte_array = (sigfox_message -> dl_payload);
	read_data.extracted_length = 0;
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
	// Build reply structure.
	reply_params.type = NODE_REPLY_TYPE_OK;
	reply_params.format = STRING_FORMAT_BOOLEAN;
	reply_params.timeout_ms = timeout_ms;
	// Send command.
	status = AT_BUS_send_command(&command_params, &reply_params, &read_data, send_status);
	if (status != NODE_SUCCESS) goto errors;
	// Read DL payload if needed.
	if ((sigfox_message -> bidirectional_flag) != 0) {
		// Flush command.
		for (idx=0 ; idx<UHFM_COMMAND_BUFFER_SIZE_BYTES ; idx++) command[idx] = STRING_CHAR_NULL;
		command_size = 0;
		// Build command.
		string_status = STRING_append_string(command, UHFM_COMMAND_BUFFER_SIZE_BYTES, UHFM_COMMAND_READ_DL_PAYLOAD, &command_size);
		STRING_status_check(NODE_ERROR_BASE_STRING);
		// Build reply structure.
		reply_params.type = NODE_REPLY_TYPE_BYTE_ARRAY;
		reply_params.format = STRING_FORMAT_HEXADECIMAL;
		reply_params.timeout_ms = AT_BUS_DEFAULT_TIMEOUT_MS;
		reply_params.byte_array_size = UHFM_SIGFOX_DL_PAYLOAD_SIZE;
		reply_params.exact_length = 1;
		// Send command.
		status = AT_BUS_send_command(&command_params, &reply_params, &read_data, send_status);
		if (status != NODE_SUCCESS) goto errors;
	}
errors:
	return status;
}
