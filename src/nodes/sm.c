/*
 * sm.c
 *
 *  Created on: 4 mar. 2023
 *      Author: Ludo
 */

#include "sm.h"

#include "at_bus.h"
#include "sm_reg.h"
#include "common.h"
#include "dinfox.h"
#include "node.h"
#include "string.h"
#include "xm.h"

/*** SM local macros ***/

#define SM_SIGFOX_PAYLOAD_MONITORING_SIZE	3
#define SM_SIGFOX_PAYLOAD_SENSOR_1_SIZE		9


/*** SM local global variables ***/

static uint32_t SM_REGISTERS[SM_REG_ADDR_LAST];

static const NODE_line_data_t SM_LINE_DATA[SM_LINE_DATA_INDEX_LAST - COMMON_LINE_DATA_INDEX_LAST] = {
	{SM_REG_ADDR_ANALOG_DATA_1, SM_REG_ANALOG_DATA_1_MASK_VAIN0, "AIN0 =", STRING_FORMAT_DECIMAL, 0, " V"},
	{SM_REG_ADDR_ANALOG_DATA_1, SM_REG_ANALOG_DATA_1_MASK_VAIN1, "AIN1 =", STRING_FORMAT_DECIMAL, 0, " V"},
	{SM_REG_ADDR_ANALOG_DATA_2, SM_REG_ANALOG_DATA_2_MASK_VAIN2, "AIN2 =", STRING_FORMAT_DECIMAL, 0, " V"},
	{SM_REG_ADDR_ANALOG_DATA_2, SM_REG_ANALOG_DATA_2_MASK_VAIN3, "AIN3 =", STRING_FORMAT_DECIMAL, 0, " V"},
	{SM_REG_ADDR_DIGITAL_DATA, SM_REG_DIGITAL_DATA_MASK_DIO0, "DIO0 =", STRING_FORMAT_BOOLEAN, 0, STRING_NULL},
	{SM_REG_ADDR_DIGITAL_DATA, SM_REG_DIGITAL_DATA_MASK_DIO1, "DIO1 =", STRING_FORMAT_BOOLEAN, 0, STRING_NULL},
	{SM_REG_ADDR_DIGITAL_DATA, SM_REG_DIGITAL_DATA_MASK_DIO2, "DIO2 =", STRING_FORMAT_BOOLEAN, 0, STRING_NULL},
	{SM_REG_ADDR_DIGITAL_DATA, SM_REG_DIGITAL_DATA_MASK_DIO3, "DIO3 =", STRING_FORMAT_BOOLEAN, 0, STRING_NULL},
	{SM_REG_ADDR_ANALOG_DATA_3, SM_REG_ANALOG_DATA_3_MASK_TAMB, "TAMB =", STRING_FORMAT_DECIMAL, 0, " |C"},
	{SM_REG_ADDR_ANALOG_DATA_3, SM_REG_ANALOG_DATA_3_MASK_HAMB, "HAMB =", STRING_FORMAT_DECIMAL, 0, " %"}
};

static const uint8_t SM_REG_ADDR_LIST_SIGFOX_PAYLOAD_MONITORING[] = {
	COMMON_REG_ADDR_ANALOG_DATA_0
};

static const uint8_t SM_REG_ADDR_LIST_SIGFOX_PAYLOAD_ELECTRICAL[] = {
	SM_REG_ADDR_ANALOG_DATA_1,
	SM_REG_ADDR_ANALOG_DATA_2,
	SM_REG_ADDR_ANALOG_DATA_3,
	SM_REG_ADDR_DIGITAL_DATA
};

/*** SM local structures ***/

typedef union {
	uint8_t frame[SM_SIGFOX_PAYLOAD_MONITORING_SIZE];
	struct {
		unsigned vmcu : 16;
		unsigned tmcu : 8;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} SM_sigfox_payload_monitoring_t;

typedef union {
	uint8_t frame[SM_SIGFOX_PAYLOAD_SENSOR_1_SIZE];
	struct {
		unsigned ain0 : 16;
		unsigned ain1 : 16;
		unsigned ain2 : 16;
		unsigned ain3 : 16;
		unsigned unused : 4;
		unsigned dio3 : 1;
		unsigned dio2 : 1;
		unsigned dio1 : 1;
		unsigned dio0 : 1;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} SM_sigfox_payload_data_t;

/*** SM functions ***/

/* WRITE SM DATA.
 * @param line_data_write:	Pointer to the data write structure.
 * @param read_status:		Pointer to the writing operation status.
 * @return status:			Function execution status.
 */
NODE_status_t SM_write_line_data(NODE_line_data_write_t* line_data_write, NODE_access_status_t* write_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	// Call common function with local data.
	status = XM_write_line_data(line_data_write, (NODE_line_data_t*) SM_LINE_DATA, (uint32_t*) SM_REG_WRITE_TIMEOUT_MS, write_status);
	return status;
}

/* READ SM DATA.
 * @param line_data_read:	Pointer to the data read structure.
 * @param read_status:		Pointer to the reading operation status.
 * @return status:			Function execution status.
 */
NODE_status_t SM_read_line_data(NODE_line_data_read_t* line_data_read, NODE_access_status_t* read_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	uint32_t field_value = 0;
	char_t field_str[STRING_DIGIT_FUNCTION_SIZE];
	uint8_t str_data_idx = 0;
	uint8_t buffer_size = 0;
	int32_t tamb = 0;
	// Check parameters.
	if (line_data_read == NULL) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	if (((line_data_read -> name_ptr) == NULL) || ((line_data_read -> value_ptr) == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Check index.
	if ((line_data_read -> line_data_index) >= SM_LINE_DATA_INDEX_LAST) {
		status = NODE_ERROR_LINE_DATA_INDEX;
		goto errors;
	}
	// Check common range.
	if ((line_data_read -> line_data_index) < COMMON_LINE_DATA_INDEX_LAST) {
		// Call common function.
		status = COMMON_read_line_data(line_data_read, (uint32_t*) SM_REGISTERS);
		if (status != NODE_SUCCESS) goto errors;
	}
	else {
		// Compute specific string data index.
		str_data_idx = ((line_data_read -> line_data_index) - COMMON_LINE_DATA_INDEX_LAST);
		// Update register.
		status = XM_read_register((line_data_read -> node_addr), SM_LINE_DATA[str_data_idx].reg_addr, (uint32_t*) SM_REGISTERS, read_status);
		if (status != NODE_SUCCESS) goto errors;
		// Compute field.
		field_value = DINFOX_read_field(SM_REGISTERS[SM_LINE_DATA[str_data_idx].reg_addr], SM_LINE_DATA[str_data_idx].field_mask);
		// Add data name.
		NODE_append_name_string((char_t*) SM_LINE_DATA[str_data_idx].name);
		buffer_size = 0;
		// Add data value.
		if ((read_status -> all) == 0) {
			// Check index.
			switch (line_data_read -> line_data_index) {
			case SM_LINE_DATA_INDEX_DIO0:
			case SM_LINE_DATA_INDEX_DIO1:
			case SM_LINE_DATA_INDEX_DIO2:
			case SM_LINE_DATA_INDEX_DIO3:
			case SM_LINE_DATA_INDEX_HAMB:
				// Specific print for boolean data.
				NODE_append_value_int32(field_value, SM_LINE_DATA[str_data_idx].print_format, SM_LINE_DATA[str_data_idx].print_prefix);
				break;
			case SM_LINE_DATA_INDEX_AIN0:
			case SM_LINE_DATA_INDEX_AIN1:
			case SM_LINE_DATA_INDEX_AIN2:
			case SM_LINE_DATA_INDEX_AIN3:
				// Convert to 5 digits string.
				string_status = STRING_value_to_5_digits_string(DINFOX_get_mv(field_value), (char_t*) field_str);
				STRING_status_check(NODE_ERROR_BASE_STRING);
				// Add string.
				NODE_append_value_string(field_str);
				break;
			case SM_LINE_DATA_INDEX_TAMB:
				// Convert temperature.
				tamb = (int32_t) DINFOX_get_degrees(field_value);
				NODE_append_value_int32(tamb, SM_LINE_DATA[str_data_idx].print_format, SM_LINE_DATA[str_data_idx].print_prefix);
				break;
			default:
				NODE_append_value_int32(field_value, STRING_FORMAT_HEXADECIMAL, 1);
				break;
			}
			// Add unit.
			NODE_append_value_string((char_t*) SM_LINE_DATA[str_data_idx].unit);
		}
		else {
			NODE_flush_string_value();
			NODE_append_value_string((char_t*) NODE_ERROR_STRING);
		}
	}
errors:
	return status;
}

/* UPDATE SM NODE SIGFOX UPLINK PAYLOAD.
 * @param ul_payload_update:	Pointer to the UL payload update structure.
 * @return status:				Function execution status.
 */
NODE_status_t SM_build_sigfox_ul_payload(NODE_ul_payload_update_t* ul_payload_update) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	NODE_access_status_t unused_write_access;
	SM_sigfox_payload_monitoring_t sigfox_payload_monitoring;
	SM_sigfox_payload_data_t sigfox_payload_data;
	uint8_t idx = 0;
	// Check parameters.
	if (ul_payload_update == NULL) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	if (((ul_payload_update -> ul_payload) == NULL) || ((ul_payload_update -> size) == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Check type.
	switch (ul_payload_update -> type) {
	case NODE_SIGFOX_PAYLOAD_TYPE_STARTUP:
		// Use common format.
		status = COMMON_build_sigfox_payload_startup(ul_payload_update, (uint32_t*) SM_REGISTERS);
		if (status != NODE_SUCCESS) goto errors;
		break;
	case NODE_SIGFOX_PAYLOAD_TYPE_MONITORING:
		// Perform measurements.
		status = XM_perform_measurements((ul_payload_update -> node_addr), &unused_write_access);
		if (status != NODE_SUCCESS) goto errors;
		// Read related registers.
		status = XM_read_registers((ul_payload_update -> node_addr), (uint8_t*) SM_REG_ADDR_LIST_SIGFOX_PAYLOAD_MONITORING, sizeof(SM_REG_ADDR_LIST_SIGFOX_PAYLOAD_MONITORING), (uint32_t*) SM_REGISTERS);
		if (status != NODE_SUCCESS) goto errors;
		// Build monitoring payload.
		sigfox_payload_monitoring.vmcu = DINFOX_read_field(SM_REGISTERS[COMMON_REG_ADDR_ANALOG_DATA_0], COMMON_REG_ANALOG_DATA_0_MASK_VMCU);
		sigfox_payload_monitoring.tmcu = DINFOX_read_field(SM_REGISTERS[COMMON_REG_ADDR_ANALOG_DATA_0], COMMON_REG_ANALOG_DATA_0_MASK_TMCU);
		// Copy payload.
		for (idx=0 ; idx<SM_SIGFOX_PAYLOAD_MONITORING_SIZE ; idx++) {
			(ul_payload_update -> ul_payload)[idx] = sigfox_payload_monitoring.frame[idx];
		}
		(*(ul_payload_update -> size)) = SM_SIGFOX_PAYLOAD_MONITORING_SIZE;
		break;
	case NODE_SIGFOX_PAYLOAD_TYPE_DATA:
		// Perform measurements.
		status = XM_perform_measurements((ul_payload_update -> node_addr), &unused_write_access);
		if (status != NODE_SUCCESS) goto errors;
		// Read related registers.
		status = XM_read_registers((ul_payload_update -> node_addr), (uint8_t*) SM_REG_ADDR_LIST_SIGFOX_PAYLOAD_ELECTRICAL, sizeof(SM_REG_ADDR_LIST_SIGFOX_PAYLOAD_ELECTRICAL), (uint32_t*) SM_REGISTERS);
		if (status != NODE_SUCCESS) goto errors;
		// Build data payload.
		sigfox_payload_data.ain0 = DINFOX_read_field(SM_REGISTERS[SM_REG_ADDR_ANALOG_DATA_1], SM_REG_ANALOG_DATA_1_MASK_VAIN0);
		sigfox_payload_data.ain1 = DINFOX_read_field(SM_REGISTERS[SM_REG_ADDR_ANALOG_DATA_1], SM_REG_ANALOG_DATA_1_MASK_VAIN1);
		sigfox_payload_data.ain2 = DINFOX_read_field(SM_REGISTERS[SM_REG_ADDR_ANALOG_DATA_2], SM_REG_ANALOG_DATA_2_MASK_VAIN2);
		sigfox_payload_data.ain3 = DINFOX_read_field(SM_REGISTERS[SM_REG_ADDR_ANALOG_DATA_2], SM_REG_ANALOG_DATA_2_MASK_VAIN3);
		sigfox_payload_data.dio0 = DINFOX_read_field(SM_REGISTERS[SM_REG_ADDR_DIGITAL_DATA], SM_REG_DIGITAL_DATA_MASK_DIO0);
		sigfox_payload_data.dio1 = DINFOX_read_field(SM_REGISTERS[SM_REG_ADDR_DIGITAL_DATA], SM_REG_DIGITAL_DATA_MASK_DIO1);
		sigfox_payload_data.dio2 = DINFOX_read_field(SM_REGISTERS[SM_REG_ADDR_DIGITAL_DATA], SM_REG_DIGITAL_DATA_MASK_DIO2);
		sigfox_payload_data.dio3 = DINFOX_read_field(SM_REGISTERS[SM_REG_ADDR_DIGITAL_DATA], SM_REG_DIGITAL_DATA_MASK_DIO3);
		sigfox_payload_data.unused = 0;
		// Copy payload.
		for (idx=0 ; idx<SM_SIGFOX_PAYLOAD_SENSOR_1_SIZE ; idx++) {
			(ul_payload_update -> ul_payload)[idx] = sigfox_payload_data.frame[idx];
		}
		(*(ul_payload_update -> size)) = SM_SIGFOX_PAYLOAD_SENSOR_1_SIZE;
		break;
	default:
		status = NODE_ERROR_SIGFOX_PAYLOAD_TYPE;
		goto errors;
	}
errors:
	return status;
}
