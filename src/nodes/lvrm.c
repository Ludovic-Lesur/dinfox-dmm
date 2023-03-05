/*
 * lvrm.c
 *
 *  Created on: 22 jan. 2023
 *      Author: Ludo
 */

#include "lvrm.h"

#include "at_bus.h"
#include "dinfox.h"
#include "string.h"

/*** LVRM local macros ***/

#define LVRM_SIGFOX_PAYLOAD_MONITORING_SIZE		3
#define LVRM_SIGFOX_PAYLOAD_DATA_SIZE			7

static const char_t* LVRM_STRING_DATA_NAME[LVRM_NUMBER_OF_SPECIFIC_STRING_DATA] = {
	"VCOM =",
	"VOUT =",
	"IOUT =",
	"RELAY ="
};
static const char_t* LVRM_STRING_DATA_UNIT[LVRM_NUMBER_OF_SPECIFIC_STRING_DATA] = {
	"mV",
	"mV",
	"uA",
	STRING_NULL
};
static const int32_t LVRM_ERROR_VALUE[LVRM_NUMBER_OF_SPECIFIC_REGISTERS] = {
	NODE_ERROR_VALUE_ANALOG_16BITS,
	NODE_ERROR_VALUE_ANALOG_16BITS,
	NODE_ERROR_VALUE_ANALOG_23BITS,
	NODE_ERROR_VALUE_BOOLEAN
};

/*** LVRM local structures ***/

typedef union {
	uint8_t frame[LVRM_SIGFOX_PAYLOAD_MONITORING_SIZE];
	struct {
		unsigned vmcu_mv : 16;
		unsigned tmcu_degrees : 8;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} LVRM_sigfox_payload_monitoring_t;

typedef union {
	uint8_t frame[LVRM_SIGFOX_PAYLOAD_DATA_SIZE];
	struct {
		unsigned vcom_mv : 16;
		unsigned vout_mv : 16;
		unsigned iout_ua : 23;
		unsigned relay_enable : 1;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} LVRM_sigfox_payload_data_t;

/*** LVRM functions ***/

/* RETRIEVE SPECIFIC DATA OF LVRM NODE.
 * @param data_update:	Pointer to the data update structure.
 * @return status:		Function execution status.
 */
NODE_status_t LVRM_update_data(NODE_data_update_t* data_update) {
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
	if (((data_update -> string_data_index) < DINFOX_STRING_DATA_INDEX_LAST) || ((data_update -> string_data_index) >= LVRM_STRING_DATA_INDEX_LAST)) {
		status = NODE_ERROR_STRING_DATA_INDEX;
		goto errors;
	}
	// Convert to register address.
	register_address = ((data_update -> string_data_index) + DINFOX_REGISTER_LAST - DINFOX_STRING_DATA_INDEX_LAST);
	// Read parameters.
	read_params.node_address = (data_update -> node_address);
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
	NODE_append_string_name((char_t*) LVRM_STRING_DATA_NAME[(data_update -> string_data_index) - DINFOX_STRING_DATA_INDEX_LAST]);
	buffer_size = 0;
	// Add data value.
	if (read_status.all == 0) {
		// Specific print for relay.
		if ((data_update -> string_data_index) == LVRM_STRING_DATA_INDEX_RELAY_ENABLE) {
			NODE_append_string_value((read_data.value == 0) ? "OFF" : "ON");
		}
		else {
			NODE_append_string_value(read_data.raw);
		}
		// Add unit.
		NODE_append_string_value((char_t*) LVRM_STRING_DATA_UNIT[(data_update -> string_data_index) - DINFOX_STRING_DATA_INDEX_LAST]);
		// Update integer data.
		NODE_update_value(register_address, read_data.value);
	}
	else {
		NODE_flush_string_value();
		NODE_append_string_value((char_t*) NODE_ERROR_STRING);
		NODE_update_value(register_address, LVRM_ERROR_VALUE[register_address]);
	}
errors:
	return status;
}

/* GET LVRM NODE SIGFOX PAYLOAD.
 * @param integer_data_value:	Pointer to the node registers value.
 * @param ul_payload_type:		Sigfox payload type.
 * @param ul_payload:			Pointer that will contain the specific sigfox payload of the node.
 * @param ul_payload_size:		Pointer to byte that will contain sigfox payload size.
 * @return status:				Function execution status.
 */
NODE_status_t LVRM_get_sigfox_ul_payload(int32_t* integer_data_value, NODE_sigfox_ul_payload_type_t ul_payload_type, uint8_t* ul_payload, uint8_t* ul_payload_size) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	LVRM_sigfox_payload_monitoring_t sigfox_payload_monitoring;
	LVRM_sigfox_payload_data_t sigfox_payload_data;
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
		// Copy payload.
		for (idx=0 ; idx<LVRM_SIGFOX_PAYLOAD_MONITORING_SIZE ; idx++) {
			ul_payload[idx] = sigfox_payload_monitoring.frame[idx];
		}
		(*ul_payload_size) = LVRM_SIGFOX_PAYLOAD_MONITORING_SIZE;
		break;
	case NODE_SIGFOX_PAYLOAD_TYPE_DATA:
		// Build data payload.
		sigfox_payload_data.vcom_mv = integer_data_value[LVRM_REGISTER_VCOM_MV];
		sigfox_payload_data.vout_mv = integer_data_value[LVRM_REGISTER_VOUT_MV];
		sigfox_payload_data.iout_ua = integer_data_value[LVRM_REGISTER_IOUT_UA];
		sigfox_payload_data.relay_enable = integer_data_value[LVRM_REGISTER_RELAY_ENABLE];
		// Copy payload.
		for (idx=0 ; idx<LVRM_SIGFOX_PAYLOAD_DATA_SIZE ; idx++) {
			ul_payload[idx] = sigfox_payload_data.frame[idx];
		}
		(*ul_payload_size) = LVRM_SIGFOX_PAYLOAD_DATA_SIZE;
		break;
	default:
		status = NODE_ERROR_SIGFOX_PAYLOAD_TYPE;
		goto errors;
	}
errors:
	return status;
}
