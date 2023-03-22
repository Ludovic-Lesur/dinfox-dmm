/*
 * dinfox.c
 *
 *  Created on: 26 jan 2023
 *      Author: Ludo
 */

#include "dinfox.h"

#include "at_bus.h"
#include "node.h"
#include "rcc_reg.h"
#include "string.h"
#include "types.h"
#include "version.h"

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
	NODE_ERROR_VALUE_NODE_ADDRESS,
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

/*** DINFOX local functions ***/

/* AT BUS READ MACRO.
 * @param:	None.
 * @return:	None.
 */
#define _DINFOX_at_bus_read(void) { \
	read_params.register_address = register_address; \
	read_params.format = DINFOX_REGISTER_FORMAT[register_address]; \
	status = AT_BUS_read_register(&read_params, &read_data, &read_status); \
	if (status != NODE_SUCCESS) goto errors; \
	if (read_status.all == 0) { \
		register_value_ptr = read_data.raw; \
		register_value_int = (int32_t) read_data.value; \
	} \
	else { \
		register_value_ptr = (char_t*) NODE_ERROR_STRING; \
		register_value_int = (int32_t) DINFOX_ERROR_VALUE[register_address]; \
		error_flag = 1; \
	} \
}

/* DMM DATA SETTING MACRO.
 * @param:	None.
 * @return:	None.
 */
#define _DINFOX_register_int_to_str(void) { \
	for (idx=0 ; idx<NODE_STRING_BUFFER_SIZE ; idx++) register_value_str[idx] = STRING_CHAR_NULL; \
	string_status = STRING_value_to_string(register_value_int, DINFOX_REGISTER_FORMAT[DINFOX_REGISTER_SW_VERSION_MAJOR], 0, register_value_str); \
	STRING_status_check(NODE_ERROR_BASE_STRING); \
	register_value_ptr = (char_t*) register_value_str; \
}

/* NODE DATA UPDATE MACRO.
 * @param:	None.
 * @return:	None.
 */
#define _DINFOX_update_value(void) { \
	if (error_flag != 0) { \
		NODE_flush_string_value(); \
	} \
	NODE_append_string_value(register_value_ptr); \
	NODE_update_value(register_address, register_value_int); \
}

/*** DINFOX functions ***/

/* UPDATE COMMON MEASUREMENTS OF DINFOX NODES.
 * @param data_update:	Pointer to the data update structure.
 * @return status:		Function execution status.
 */
NODE_status_t DINFOX_update_data(NODE_data_update_t* data_update) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	ADC_status_t adc1_status = ADC_SUCCESS;
	NODE_read_parameters_t read_params;
	NODE_read_data_t read_data;
	NODE_access_status_t read_status;
	uint8_t buffer_size = 0;
	uint8_t register_address = 0;
	int32_t register_value_int = 0;
	char_t register_value_str[NODE_STRING_BUFFER_SIZE] = {STRING_CHAR_NULL};
	char_t* register_value_ptr;
	uint32_t generic_u32 = 0;
	int8_t generic_s8 = 0;
	uint8_t idx = 0;
	uint8_t error_flag = 0;
	// Common reply parameters.
	read_params.node_address = (data_update -> node_address);
	read_params.type = NODE_REPLY_TYPE_VALUE;
	read_params.timeout_ms = AT_BUS_DEFAULT_TIMEOUT_MS;
	// Configure read data.
	read_data.raw = NULL;
	read_data.value = 0;
	read_data.byte_array = NULL;
	read_data.extracted_length = 0;
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
	if ((data_update -> string_data_index) >= DINFOX_STRING_DATA_INDEX_LAST) {
		status = NODE_ERROR_STRING_DATA_INDEX;
		goto errors;
	}
	// Add data name.
	NODE_append_string_name((char_t*) DINFOX_STRING_DATA_NAME[data_update -> string_data_index]);
	buffer_size = 0;
	// Check index.
	switch (data_update -> string_data_index) {
	case DINFOX_STRING_DATA_INDEX_HW_VERSION:
		// Hardware version major.
		register_address = DINFOX_REGISTER_HW_VERSION_MAJOR;
		// Check register address.
		if ((data_update -> node_address) != DINFOX_NODE_ADDRESS_DMM) {
			_DINFOX_at_bus_read();
		}
		else {
#ifdef HW1_0
			register_value_int = (int32_t) 1;
#else
			register_value_int = (int32_t) DINFOX_ERROR_VALUE[register_address];
#endif
			_DINFOX_register_int_to_str();
		}
		_DINFOX_update_value();
		// Hardware version minor.
		register_address = DINFOX_REGISTER_HW_VERSION_MINOR;
		// Check register address.
		if ((data_update -> node_address) != DINFOX_NODE_ADDRESS_DMM) {
			_DINFOX_at_bus_read();
		}
		else {
#ifdef HW1_0
			register_value_int = (int32_t) 0;
#else
			register_value_int = (int32_t) NODE_ERROR_VALUE_VERSION;
#endif
			_DINFOX_register_int_to_str();
		}
		NODE_append_string_value(".");
		_DINFOX_update_value();
		break;
	case DINFOX_STRING_DATA_INDEX_SW_VERSION:
		// Software version major.
		register_address = DINFOX_REGISTER_SW_VERSION_MAJOR;
		// Check register address.
		if ((data_update -> node_address) != DINFOX_NODE_ADDRESS_DMM) {
			_DINFOX_at_bus_read();
		}
		else {
			// Read value.
			register_value_int = (int32_t) GIT_MAJOR_VERSION;
			_DINFOX_register_int_to_str();
		}
		_DINFOX_update_value();
		// Software version minor.
		register_address = DINFOX_REGISTER_SW_VERSION_MINOR;
		// Check register address.
		if ((data_update -> node_address) != DINFOX_NODE_ADDRESS_DMM) {
			_DINFOX_at_bus_read();
		}
		else {
			register_value_int = (int32_t) GIT_MINOR_VERSION;
			_DINFOX_register_int_to_str();
		}
		NODE_append_string_value(".");
		_DINFOX_update_value();
		// Software version commit index.
		register_address = DINFOX_REGISTER_SW_VERSION_COMMIT_INDEX;
		// Check register address.
		if ((data_update -> node_address) != DINFOX_NODE_ADDRESS_DMM) {
			_DINFOX_at_bus_read();
		}
		else {
			// Read value.
			register_value_int = (int32_t) GIT_COMMIT_INDEX;
			_DINFOX_register_int_to_str();
		}
		NODE_append_string_value(".");
		_DINFOX_update_value();
		// Software version commit ID.
		register_address = DINFOX_REGISTER_SW_VERSION_COMMIT_ID;
		// Check register address.
		if ((data_update -> node_address) != DINFOX_NODE_ADDRESS_DMM) {
			_DINFOX_at_bus_read();
		}
		else {
			register_value_int = (int32_t) GIT_COMMIT_ID;
			_DINFOX_register_int_to_str();
		}
		NODE_update_value(register_address, register_value_int);
		// Software version dirty flag.
		register_address = DINFOX_REGISTER_SW_VERSION_DIRTY_FLAG;
		// Check register address.
		if ((data_update -> node_address) != DINFOX_NODE_ADDRESS_DMM) {
			_DINFOX_at_bus_read();
		}
		else {
			// Read value.
			register_value_int = (int32_t) GIT_DIRTY_FLAG;
			_DINFOX_register_int_to_str();
		}
		if (register_value_int != 0) {
			NODE_append_string_value(".d");
		}
		NODE_update_value(register_address, register_value_int);
		break;
	case DINFOX_STRING_DATA_INDEX_RESET_REASON:
		// Reset flags.
		register_address = DINFOX_REGISTER_RESET_REASON;
		// Check register address.
		if ((data_update -> node_address) != DINFOX_NODE_ADDRESS_DMM) {
			_DINFOX_at_bus_read();
		}
		else {
			// Read value.
			register_value_int = (int32_t) (((RCC -> CSR) >> 24) & 0xFF);
			_DINFOX_register_int_to_str();
		}
		NODE_append_string_value("0x");
		_DINFOX_update_value();
		break;
	case DINFOX_STRING_DATA_INDEX_TMCU_DEGREES:
		// MCU temperature.
		register_address = DINFOX_REGISTER_TMCU_DEGREES;
		// Check register address.
		if ((data_update -> node_address) != DINFOX_NODE_ADDRESS_DMM) {
			_DINFOX_at_bus_read();
		}
		else {
			adc1_status = ADC1_get_tmcu(&generic_s8);
			ADC1_status_check(NODE_ERROR_BASE_ADC);
			register_value_int = (int32_t) generic_s8;
			_DINFOX_register_int_to_str();
		}
		_DINFOX_update_value();
		break;
	case DINFOX_STRING_DATA_INDEX_VMCU_MV:
		// MCU voltage.
		register_address = DINFOX_REGISTER_VMCU_MV;
		// Check register address.
		if ((data_update -> node_address) != DINFOX_NODE_ADDRESS_DMM) {
			_DINFOX_at_bus_read();
		}
		else {
			adc1_status = ADC1_get_data(ADC_DATA_INDEX_VMCU_MV, &generic_u32);
			ADC1_status_check(NODE_ERROR_BASE_ADC);
			register_value_int = (int32_t) generic_u32;
			_DINFOX_register_int_to_str();
		}
		_DINFOX_update_value();
		break;
	default:
		status = NODE_ERROR_STRING_DATA_INDEX;
		goto errors;
	}
	// Add unit if no error.
	NODE_append_string_value((char_t*) DINFOX_STRING_DATA_UNIT[data_update -> string_data_index]);
errors:
	return status;
}
