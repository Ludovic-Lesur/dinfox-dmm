/*
 * sm.c
 *
 *  Created on: 4 mar. 2023
 *      Author: Ludo
 */

#include "sm.h"

#include "at_bus.h"
#include "dinfox.h"
#include "string.h"

/*** SM local macros ***/

#define SM_SIGFOX_PAYLOAD_MONITORING_SIZE	3
#define SM_SIGFOX_PAYLOAD_SENSOR_SIZE		10

static const char_t* SM_STRING_DATA_NAME[SM_NUMBER_OF_SPECIFIC_STRING_DATA] = {
	"AIN0 =",
	"AIN1 =",
	"AIN2 =",
	"AIN3 =",
	"DIO0 =",
	"DIO1 =",
	"DIO2 =",
	"DIO3 =",
	"TAMB =",
	"HAMB =",
};
static const char_t* SM_STRING_DATA_UNIT[SM_NUMBER_OF_SPECIFIC_STRING_DATA] = {
	"mV",
	"mV",
	"mV",
	"mV",
	STRING_NULL,
	STRING_NULL,
	STRING_NULL,
	STRING_NULL,
	"|C",
	"%"
};
static const int32_t SM_ERROR_VALUE[SM_NUMBER_OF_SPECIFIC_REGISTERS] = {
	NODE_ERROR_VALUE_ANALOG_16BITS,
	NODE_ERROR_VALUE_ANALOG_16BITS,
	NODE_ERROR_VALUE_ANALOG_16BITS,
	NODE_ERROR_VALUE_ANALOG_16BITS,
	NODE_ERROR_VALUE_BOOLEAN,
	NODE_ERROR_VALUE_BOOLEAN,
	NODE_ERROR_VALUE_BOOLEAN,
	NODE_ERROR_VALUE_BOOLEAN,
	NODE_ERROR_VALUE_TEMPERATURE,
	NODE_ERROR_VALUE_HUMIDITY
};

/*** SM local structures ***/

typedef union {
	uint8_t frame[SM_SIGFOX_PAYLOAD_MONITORING_SIZE];
	struct {
		unsigned vmcu_mv : 16;
		unsigned tmcu_degrees : 8;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} SM_sigfox_payload_monitoring_t;

typedef union {
	uint8_t frame[SM_SIGFOX_PAYLOAD_SENSOR_SIZE];
	struct {
		unsigned ain0_mv : 15;
		unsigned dio0 : 1;
		unsigned ain1_mv : 15;
		unsigned dio1 : 1;
		unsigned ain2_mv : 15;
		unsigned dio2 : 1;
		unsigned ain3_mv : 15;
		unsigned dio3 : 1;
		unsigned tamb_degrees : 8;
		unsigned hamb_percent : 8;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} SM_sigfox_payload_data_t;

/*** SM functions ***/

/* RETRIEVE SPECIFIC DATA OF SM NODE.
 * @param data_update:	Pointer to the data update structure.
 * @return status:		Function execution status.
 */
NODE_status_t SM_update_data(NODE_data_update_t* data_update) {
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
	if (((data_update -> string_data_index) < DINFOX_STRING_DATA_INDEX_LAST) || ((data_update -> string_data_index) >= SM_STRING_DATA_INDEX_LAST)) {
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
	NODE_append_string_name((char_t*) SM_STRING_DATA_NAME[(data_update -> string_data_index) - DINFOX_STRING_DATA_INDEX_LAST]);
	buffer_size = 0;
	// Add data value.
	if (read_status.all == 0) {
		// Print value.
		NODE_append_string_value(read_data.raw);
		// Add unit.
		NODE_append_string_value((char_t*) SM_STRING_DATA_UNIT[(data_update -> string_data_index) - DINFOX_STRING_DATA_INDEX_LAST]);
		// Update integer data.
		NODE_update_value(register_address, read_data.value);
	}
	else {
		NODE_flush_string_value();
		NODE_append_string_value((char_t*) NODE_ERROR_STRING);
		NODE_update_value(register_address, SM_ERROR_VALUE[register_address - DINFOX_REGISTER_LAST]);
	}
errors:
	return status;
}

/* GET SM NODE SIGFOX PAYLOAD.
 * @param integer_data_value:	Pointer to the node registers value.
 * @param ul_payload_type:		Sigfox payload type.
 * @param ul_payload:			Pointer that will contain the specific sigfox payload of the node.
 * @param ul_payload_size:		Pointer to byte that will contain sigfox payload size.
 * @return status:				Function execution status.
 */
NODE_status_t SM_get_sigfox_ul_payload(int32_t* integer_data_value, NODE_sigfox_ul_payload_type_t ul_payload_type, uint8_t* ul_payload, uint8_t* ul_payload_size) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	SM_sigfox_payload_monitoring_t sigfox_payload_monitoring;
	SM_sigfox_payload_data_t sigfox_payload_data;
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
		for (idx=0 ; idx<SM_SIGFOX_PAYLOAD_MONITORING_SIZE ; idx++) {
			ul_payload[idx] = sigfox_payload_monitoring.frame[idx];
		}
		(*ul_payload_size) = SM_SIGFOX_PAYLOAD_MONITORING_SIZE;
		break;
	case NODE_SIGFOX_PAYLOAD_TYPE_DATA:
		// Build data payload.
		sigfox_payload_data.ain0_mv = integer_data_value[SM_REGISTER_AIN0_MV] & 0x7FFF;
		sigfox_payload_data.ain1_mv = integer_data_value[SM_REGISTER_AIN1_MV] & 0x7FFF;
		sigfox_payload_data.ain2_mv = integer_data_value[SM_REGISTER_AIN2_MV] & 0x7FFF;
		sigfox_payload_data.ain3_mv = integer_data_value[SM_REGISTER_AIN3_MV] & 0x7FFF;
		sigfox_payload_data.dio0 = integer_data_value[SM_REGISTER_DIO0] & 0x01;
		sigfox_payload_data.dio1 = integer_data_value[SM_REGISTER_DIO1] & 0x01;
		sigfox_payload_data.dio2 = integer_data_value[SM_REGISTER_DIO2] & 0x01;
		sigfox_payload_data.dio3 = integer_data_value[SM_REGISTER_DIO3] & 0x01;
		sigfox_payload_data.tamb_degrees = integer_data_value[SM_REGISTER_TAMB_DEGREES];
		sigfox_payload_data.hamb_percent = integer_data_value[SM_REGISTER_HAMB_PERCENT];
		// Copy payload.
		for (idx=0 ; idx<SM_SIGFOX_PAYLOAD_SENSOR_SIZE ; idx++) {
			ul_payload[idx] = sigfox_payload_data.frame[idx];
		}
		(*ul_payload_size) = SM_SIGFOX_PAYLOAD_SENSOR_SIZE;
		break;
	default:
		status = NODE_ERROR_SIGFOX_PAYLOAD_TYPE;
		goto errors;
	}
errors:
	return status;
}

