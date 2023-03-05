/*
 * dmm.c
 *
 *  Created on: 4 mar. 2023
 *      Author: Ludo
 */

#include "dmm.h"

#include "adc.h"
#include "dinfox.h"
#include "node.h"
#include "nvm.h"
#include "string.h"

/*** DMM local macros ***/

#define DMM_SIGFOX_PAYLOAD_MONITORING_SIZE		10

static const char_t* DMM_STRING_DATA_NAME[DMM_NUMBER_OF_SPECIFIC_STRING_DATA] = {
	"VUSB =",
	"VRS =",
	"VHMI =",
	"NODES_CNT =",
	"UL_PRD = ",
	"DL_PRD = "
};
static const char_t* DMM_STRING_DATA_UNIT[DMM_NUMBER_OF_SPECIFIC_STRING_DATA] = {
	"mV",
	"mV",
	"mV",
	STRING_NULL,
	"s",
	"s"
};
static const int32_t DMM_ERROR_VALUE[DMM_NUMBER_OF_SPECIFIC_REGISTERS] = {
	NODE_ERROR_VALUE_ANALOG_16BITS,
	NODE_ERROR_VALUE_ANALOG_16BITS,
	NODE_ERROR_VALUE_ANALOG_16BITS,
	0,
	0,
	0
};

/*** DMM local structures ***/

typedef union {
	uint8_t frame[DMM_SIGFOX_PAYLOAD_MONITORING_SIZE];
	struct {
		unsigned vmcu_mv : 16;
		unsigned tmcu_degrees : 8;
		unsigned vusb_mv : 16;
		unsigned vrs_mv : 16;
		unsigned vhmi_mv : 16;
		unsigned nodes_count : 8;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} DMM_sigfox_payload_monitoring_t;

/*** DMM functions ***/

/* READ DMM REGISTER.
 * @param read_params:	Pointer to the read operation parameters.
 * @param read_data:	Pointer to the read result.
 * @param read_status:	Pointer to the read operation status.
 * @return status:		Function execution status.
 */
NODE_status_t DMM_read_register(NODE_read_parameters_t* read_params, NODE_read_data_t* read_data, NODE_access_status_t* read_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	return status;
}

/* WRITE AT BUS NODE REGISTER.
 * @param write_params:	Pointer to the write operation parameters.
 * @param write_status:	Pointer to the write operation status.
 * @return status:		Function execution status.
 */
NODE_status_t DMM_write_register(NODE_write_parameters_t* write_params, NODE_access_status_t* write_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	return status;
}

/* RETRIEVE SPECIFIC DATA OF DMM NODE.
 * @param data_update:	Pointer to the data update structure.
 * @return status:		Function execution status.
 */
NODE_status_t DMM_update_data(NODE_data_update_t* data_update) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	ADC_status_t adc1_status = ADC_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	uint8_t register_address = 0;
	uint8_t buffer_size = 0;
	uint32_t generic_u32 = 0;
	int32_t register_value_int = 0;
	char_t register_value_str[NODE_STRING_BUFFER_SIZE] = {STRING_CHAR_NULL};
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
	if (((data_update -> string_data_index) < DINFOX_STRING_DATA_INDEX_LAST) || ((data_update -> string_data_index) >= DMM_STRING_DATA_INDEX_LAST)) {
		status = NODE_ERROR_STRING_DATA_INDEX;
		goto errors;
	}
	// Convert to register address.
	register_address = ((data_update -> string_data_index) + DINFOX_REGISTER_LAST - DINFOX_STRING_DATA_INDEX_LAST);
	// Add data name.
	NODE_append_string_name((char_t*) DMM_STRING_DATA_NAME[(data_update -> string_data_index) - DINFOX_STRING_DATA_INDEX_LAST]);
	buffer_size = 0;
	// Read data.
	switch (register_address) {
	case DMM_REGISTER_VUSB_MV:
	case DMM_REGISTER_VRS_MV:
	case DMM_REGISTER_VHMI_MV:
		// Note: indexing only works if registers addresses are ordered in the same way as ADC data indexes.
		adc1_status = ADC1_get_data((ADC_DATA_INDEX_VUSB_MV + (register_address - DMM_REGISTER_VUSB_MV)), &generic_u32);
		ADC1_status_check(NODE_ERROR_BASE_ADC);
		register_value_int = (int32_t) generic_u32;
		break;
	case DMM_REGISTER_NODES_COUNT:
		register_value_int = (int32_t) NODES_LIST.count;
		break;
	case DMM_REGISTER_SIGFOX_UL_PERIOD_SECONDS:
		register_value_int = (int32_t) NODE_get_sigfox_ul_period();
		break;
	case DMM_REGISTER_SIGFOX_DL_PERIOD_SECONDS:
		register_value_int = (int32_t) NODE_get_sigfox_dl_period();
		break;
	default:
		status = NODE_ERROR_STRING_DATA_INDEX;
		goto errors;
	}
	// Convert value to string.
	string_status = STRING_value_to_string(register_value_int, DMM_REGISTERS_FORMAT[(data_update -> string_data_index) - DINFOX_STRING_DATA_INDEX_LAST], 0, register_value_str);
	STRING_status_check(NODE_ERROR_BASE_STRING);
	// Add value and unit.
	NODE_append_string_value(register_value_str);
	NODE_append_string_value((char_t*) DMM_STRING_DATA_UNIT[(data_update -> string_data_index) - DINFOX_STRING_DATA_INDEX_LAST]);
	// Update integer data.
	NODE_update_value(register_address, register_value_int);
	return NODE_SUCCESS;
errors:
	NODE_flush_string_value();
	NODE_append_string_value((char_t*) NODE_ERROR_STRING);
	NODE_update_value(register_address, DMM_ERROR_VALUE[register_address]);
	return status;
}

/* GET DMM NODE SIGFOX PAYLOAD.
 * @param integer_data_value:	Pointer to the node registers value.
 * @param ul_payload_type:		Sigfox payload type.
 * @param ul_payload:			Pointer that will contain the specific sigfox payload of the node.
 * @param ul_payload_size:		Pointer to byte that will contain sigfox payload size.
 * @return status:				Function execution status.
 */
NODE_status_t DMM_get_sigfox_ul_payload(int32_t* integer_data_value, NODE_sigfox_ul_payload_type_t ul_payload_type, uint8_t* ul_payload, uint8_t* ul_payload_size) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	DMM_sigfox_payload_monitoring_t sigfox_payload_monitoring;
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
		sigfox_payload_monitoring.vusb_mv = integer_data_value[DMM_REGISTER_VUSB_MV];
		sigfox_payload_monitoring.vrs_mv = integer_data_value[DMM_REGISTER_VRS_MV];
		sigfox_payload_monitoring.vhmi_mv = integer_data_value[DMM_REGISTER_VHMI_MV];
		sigfox_payload_monitoring.nodes_count = integer_data_value[DMM_REGISTER_NODES_COUNT];
		// Copy payload.
		for (idx=0 ; idx<DMM_SIGFOX_PAYLOAD_MONITORING_SIZE ; idx++) {
			ul_payload[idx] = sigfox_payload_monitoring.frame[idx];
		}
		(*ul_payload_size) = DMM_SIGFOX_PAYLOAD_MONITORING_SIZE;
		break;
	default:
		status = NODE_ERROR_SIGFOX_PAYLOAD_TYPE;
		goto errors;
	}
errors:
	return status;
}

