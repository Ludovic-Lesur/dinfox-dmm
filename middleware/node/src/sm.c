/*
 * sm.c
 *
 *  Created on: 04 mar. 2023
 *      Author: Ludo
 */

#include "sm.h"

#include "common.h"
#include "common_reg.h"
#include "dinfox.h"
#include "node.h"
#include "node_common.h"
#include "sm_reg.h"
#include "string.h"
#include "xm.h"

/*** SM local macros ***/

#define SM_SIGFOX_UL_PAYLOAD_MONITORING_SIZE	3
#define SM_SIGFOX_UL_PAYLOAD_ELECTRICAL_SIZE	9
#define SM_SIGFOX_UL_PAYLOAD_SENSOR_SIZE		2

#define SM_SIGFOX_UL_PAYLOAD_LOOP_MAX			5

/*** SM local structures ***/

/*******************************************************************/
typedef enum {
	SM_SIGFOX_UL_PAYLOAD_TYPE_MONITORING = 0,
	SM_SIGFOX_UL_PAYLOAD_TYPE_ELECTRICAL,
	SM_SIGFOX_UL_PAYLOAD_TYPE_SENSOR,
	SM_SIGFOX_UL_PAYLOAD_TYPE_LAST
} SM_sigfox_ul_payload_type_t;

/*******************************************************************/
typedef union {
	uint8_t frame[SM_SIGFOX_UL_PAYLOAD_MONITORING_SIZE];
	struct {
		unsigned vmcu : 16;
		unsigned tmcu : 8;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} SM_sigfox_ul_payload_monitoring_t;

/*******************************************************************/
typedef union {
	uint8_t frame[SM_SIGFOX_UL_PAYLOAD_ELECTRICAL_SIZE];
	struct {
		unsigned ain0 : 16;
		unsigned ain1 : 16;
		unsigned ain2 : 16;
		unsigned ain3 : 16;
		unsigned dio3 : 2;
		unsigned dio2 : 2;
		unsigned dio1 : 2;
		unsigned dio0 : 2;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} SM_sigfox_ul_payload_electrical_t;

/*******************************************************************/
typedef union {
	uint8_t frame[SM_SIGFOX_UL_PAYLOAD_SENSOR_SIZE];
	struct {
		unsigned tamb : 8;
		unsigned hamb : 8;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} SM_sigfox_ul_payload_sensor_t;

/*** SM global variables ***/

const uint32_t SM_REG_WRITE_TIMEOUT_MS[SM_REG_ADDR_LAST] = {
	COMMON_REG_WRITE_TIMEOUT_MS_LIST
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS
};

/*** SM local global variables ***/

static uint32_t SM_REGISTERS[SM_REG_ADDR_LAST];

static const uint32_t SM_REG_ERROR_VALUE[SM_REG_ADDR_LAST] = {
	COMMON_REG_ERROR_VALUE_LIST
	0x00000000,
	0x00000000,
	0x00000000,
	(uint32_t) ((DINFOX_VOLTAGE_ERROR_VALUE << 16) | (DINFOX_VOLTAGE_ERROR_VALUE << 0)),
	(uint32_t) ((DINFOX_VOLTAGE_ERROR_VALUE << 16) | (DINFOX_VOLTAGE_ERROR_VALUE << 0)),
	(uint32_t) ((DINFOX_HUMIDITY_ERROR_VALUE << 8) | (DINFOX_TEMPERATURE_ERROR_VALUE << 0)),
	((DINFOX_BIT_ERROR << 6) | (DINFOX_BIT_ERROR << 4) | (DINFOX_BIT_ERROR << 2) | (DINFOX_BIT_ERROR << 0)),
};

static const XM_node_registers_t SM_NODE_REGISTERS = {
	.value = (uint32_t*) SM_REGISTERS,
	.error = (uint32_t*) SM_REG_ERROR_VALUE,
};

static const NODE_line_data_t SM_LINE_DATA[SM_LINE_DATA_INDEX_LAST - COMMON_LINE_DATA_INDEX_LAST] = {
	{"AIN0 =", " V", STRING_FORMAT_DECIMAL, 0, SM_REG_ADDR_ANALOG_DATA_1, SM_REG_ANALOG_DATA_1_MASK_VAIN0,     COMMON_REG_ADDR_CONTROL_0, DINFOX_REG_MASK_NONE},
	{"AIN1 =", " V", STRING_FORMAT_DECIMAL, 0, SM_REG_ADDR_ANALOG_DATA_1, SM_REG_ANALOG_DATA_1_MASK_VAIN1,     COMMON_REG_ADDR_CONTROL_0, DINFOX_REG_MASK_NONE},
	{"AIN2 =", " V", STRING_FORMAT_DECIMAL, 0, SM_REG_ADDR_ANALOG_DATA_2, SM_REG_ANALOG_DATA_2_MASK_VAIN2,     COMMON_REG_ADDR_CONTROL_0, DINFOX_REG_MASK_NONE},
	{"AIN3 =", " V", STRING_FORMAT_DECIMAL, 0, SM_REG_ADDR_ANALOG_DATA_2, SM_REG_ANALOG_DATA_2_MASK_VAIN3,     COMMON_REG_ADDR_CONTROL_0, DINFOX_REG_MASK_NONE},
	{"DIO0 =", STRING_NULL, STRING_FORMAT_DECIMAL, 0, SM_REG_ADDR_DIGITAL_DATA, SM_REG_DIGITAL_DATA_MASK_DIO0, COMMON_REG_ADDR_CONTROL_0, DINFOX_REG_MASK_NONE},
	{"DIO1 =", STRING_NULL, STRING_FORMAT_DECIMAL, 0, SM_REG_ADDR_DIGITAL_DATA, SM_REG_DIGITAL_DATA_MASK_DIO1, COMMON_REG_ADDR_CONTROL_0, DINFOX_REG_MASK_NONE},
	{"DIO2 =", STRING_NULL, STRING_FORMAT_DECIMAL, 0, SM_REG_ADDR_DIGITAL_DATA, SM_REG_DIGITAL_DATA_MASK_DIO2, COMMON_REG_ADDR_CONTROL_0, DINFOX_REG_MASK_NONE},
	{"DIO3 =", STRING_NULL, STRING_FORMAT_DECIMAL, 0, SM_REG_ADDR_DIGITAL_DATA, SM_REG_DIGITAL_DATA_MASK_DIO3, COMMON_REG_ADDR_CONTROL_0, DINFOX_REG_MASK_NONE},
	{"TAMB =", " |C", STRING_FORMAT_DECIMAL, 0, SM_REG_ADDR_ANALOG_DATA_3, SM_REG_ANALOG_DATA_3_MASK_TAMB,     COMMON_REG_ADDR_CONTROL_0, DINFOX_REG_MASK_NONE},
	{"HAMB =", " %",  STRING_FORMAT_DECIMAL, 0, SM_REG_ADDR_ANALOG_DATA_3, SM_REG_ANALOG_DATA_3_MASK_HAMB,     COMMON_REG_ADDR_CONTROL_0, DINFOX_REG_MASK_NONE}
};

static const uint8_t SM_REG_LIST_SIGFOX_UL_PAYLOAD_MONITORING[] = {
	COMMON_REG_ADDR_ANALOG_DATA_0,
};

static const uint8_t SM_REG_LIST_SIGFOX_UL_PAYLOAD_ELECTRICAL[] = {
	SM_REG_ADDR_ANALOG_DATA_1,
	SM_REG_ADDR_ANALOG_DATA_2,
	SM_REG_ADDR_DIGITAL_DATA
};

static const uint8_t SM_REG_LIST_SIGFOX_UL_PAYLOAD_SENSOR[] = {
	SM_REG_ADDR_ANALOG_DATA_3
};

static const SM_sigfox_ul_payload_type_t SM_SIGFOX_UL_PAYLOAD_PATTERN[] = {
	SM_SIGFOX_UL_PAYLOAD_TYPE_ELECTRICAL,
	SM_SIGFOX_UL_PAYLOAD_TYPE_SENSOR,
	SM_SIGFOX_UL_PAYLOAD_TYPE_ELECTRICAL,
	SM_SIGFOX_UL_PAYLOAD_TYPE_SENSOR,
	SM_SIGFOX_UL_PAYLOAD_TYPE_ELECTRICAL,
	SM_SIGFOX_UL_PAYLOAD_TYPE_SENSOR,
	SM_SIGFOX_UL_PAYLOAD_TYPE_ELECTRICAL,
	SM_SIGFOX_UL_PAYLOAD_TYPE_SENSOR,
	SM_SIGFOX_UL_PAYLOAD_TYPE_ELECTRICAL,
	SM_SIGFOX_UL_PAYLOAD_TYPE_SENSOR,
	SM_SIGFOX_UL_PAYLOAD_TYPE_MONITORING
};

/*** SM functions ***/

/*******************************************************************/
NODE_status_t SM_write_line_data(NODE_line_data_write_t* line_data_write, NODE_access_status_t* write_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	// Check parameters.
	if ((line_data_write == NULL) || (write_status == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Check index.
	if ((line_data_write -> line_data_index) >= SM_LINE_DATA_INDEX_LAST) {
		status = NODE_ERROR_LINE_DATA_INDEX;
		goto errors;
	}
	// Check common range.
	if ((line_data_write -> line_data_index) < COMMON_LINE_DATA_INDEX_LAST) {
		// Call common function.
		status = COMMON_write_line_data(line_data_write, write_status);
	}
	else {
		// Remove offset.
		(line_data_write -> line_data_index) -= COMMON_LINE_DATA_INDEX_LAST;
		// Call common function.
		status = XM_write_line_data(line_data_write, (NODE_line_data_t*) SM_LINE_DATA, (uint32_t*) SM_REG_WRITE_TIMEOUT_MS, write_status);
	}
errors:
	return status;
}

/*******************************************************************/
NODE_status_t SM_read_line_data(NODE_line_data_read_t* line_data_read, NODE_access_status_t* read_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	XM_node_registers_t node_reg;
	STRING_status_t string_status = STRING_SUCCESS;
	uint32_t field_value = 0;
	char_t field_str[STRING_DIGIT_FUNCTION_SIZE];
	uint8_t str_data_idx = 0;
	uint8_t reg_addr = 0;
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
	// Build node registers structure.
	node_reg.value = (uint32_t*) SM_REGISTERS;
	node_reg.error = (uint32_t*) SM_REG_ERROR_VALUE;
	// Check common range.
	if ((line_data_read -> line_data_index) < COMMON_LINE_DATA_INDEX_LAST) {
		// Call common function.
		status = COMMON_read_line_data(line_data_read, &node_reg, read_status);
		if (status != NODE_SUCCESS) goto errors;
	}
	else {
		// Compute specific string data index and register address.
		str_data_idx = ((line_data_read -> line_data_index) - COMMON_LINE_DATA_INDEX_LAST);
		reg_addr = SM_LINE_DATA[str_data_idx].read_reg_addr;
		// Add data name.
		NODE_append_name_string((char_t*) SM_LINE_DATA[str_data_idx].name);
		buffer_size = 0;
		// Reset result to error.
		NODE_flush_string_value();
		NODE_append_value_string((char_t*) NODE_ERROR_STRING);
		// Update register.
		status = XM_read_register((line_data_read -> node_addr), reg_addr, (XM_node_registers_t*) &SM_NODE_REGISTERS, read_status);
		if ((status != NODE_SUCCESS) || ((read_status -> flags) != 0)) goto errors;
		// Compute field.
		field_value = DINFOX_read_field(SM_REGISTERS[reg_addr], SM_LINE_DATA[str_data_idx].read_field_mask);
		// Check index.
		switch (line_data_read -> line_data_index) {
		case SM_LINE_DATA_INDEX_DIO0:
		case SM_LINE_DATA_INDEX_DIO1:
		case SM_LINE_DATA_INDEX_DIO2:
		case SM_LINE_DATA_INDEX_DIO3:
			// Specific print for boolean data.
			NODE_flush_string_value();
			switch (field_value) {
			case DINFOX_BIT_0:
				NODE_append_value_string("LOW");
				break;
			case DINFOX_BIT_1:
				NODE_append_value_string("HIGH");
				break;
			case DINFOX_BIT_FORCED_HARDWARE:
				NODE_append_value_string("HW");
				break;
			default:
				NODE_append_value_string("ERROR");
				break;
			}
			break;
		case SM_LINE_DATA_INDEX_HAMB:
			// Specific print for humidity.
			NODE_flush_string_value();
			NODE_append_value_int32(field_value, SM_LINE_DATA[str_data_idx].print_format, SM_LINE_DATA[str_data_idx].print_prefix);
			// Add unit.
			NODE_append_value_string((char_t*) SM_LINE_DATA[str_data_idx].unit);
			break;
		case SM_LINE_DATA_INDEX_AIN0:
		case SM_LINE_DATA_INDEX_AIN1:
		case SM_LINE_DATA_INDEX_AIN2:
		case SM_LINE_DATA_INDEX_AIN3:
			// Check error value.
			if (field_value != DINFOX_VOLTAGE_ERROR_VALUE) {
				// Convert to 5 digits string.
				string_status = STRING_value_to_5_digits_string((int32_t) (DINFOX_get_mv((DINFOX_voltage_representation_t) field_value)), (char_t*) field_str);
				STRING_exit_error(NODE_ERROR_BASE_STRING);
				// Add string.
				NODE_flush_string_value();
				NODE_append_value_string(field_str);
				// Add unit.
				NODE_append_value_string((char_t*) SM_LINE_DATA[str_data_idx].unit);
			}
			break;
		case SM_LINE_DATA_INDEX_TAMB:
			// Check error value.
			if (field_value != DINFOX_TEMPERATURE_ERROR_VALUE) {
				// Convert temperature.
				tamb = (int32_t) DINFOX_get_degrees((DINFOX_temperature_representation_t) field_value);
				// Add string.
				NODE_flush_string_value();
				NODE_append_value_int32(tamb, SM_LINE_DATA[str_data_idx].print_format, SM_LINE_DATA[str_data_idx].print_prefix);
				// Add unit.
				NODE_append_value_string((char_t*) SM_LINE_DATA[str_data_idx].unit);
			}
			break;
		default:
			NODE_flush_string_value();
			NODE_append_value_int32(field_value, STRING_FORMAT_HEXADECIMAL, 1);
			break;
		}
	}
errors:
	return status;
}

/*******************************************************************/
NODE_status_t SM_build_sigfox_ul_payload(NODE_ul_payload_t* node_ul_payload) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	NODE_access_status_t access_status;
	XM_registers_list_t reg_list;
	SM_sigfox_ul_payload_monitoring_t sigfox_ul_payload_monitoring;
	SM_sigfox_ul_payload_electrical_t sigfox_ul_payload_electrical;
	SM_sigfox_ul_payload_sensor_t sigfox_ul_payload_sensor;
	uint32_t reg_configuration = 0;
	uint8_t idx = 0;
	uint32_t loop_count = 0;
	// Check parameters.
	if (node_ul_payload == NULL) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	if (((node_ul_payload -> node) == NULL) || ((node_ul_payload -> ul_payload) == NULL) || ((node_ul_payload -> size) == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Reset payload size.
	(*(node_ul_payload -> size)) = 0;
	// Check event driven payloads.
	status = COMMON_check_event_driven_payloads(node_ul_payload, (XM_node_registers_t*) &SM_NODE_REGISTERS);
	if (status != NODE_SUCCESS) goto errors;
	// Directly exits if a common payload was computed.
	if ((*(node_ul_payload -> size)) > 0) goto errors;
	// Else use specific pattern of the node.
	status = XM_read_register((node_ul_payload -> node -> address), SM_REG_ADDR_CONFIGURATION_0, (XM_node_registers_t*) &SM_NODE_REGISTERS, &access_status);
	if ((status != NODE_SUCCESS) || (access_status.flags != 0)) goto errors;
	// Update local value.
	reg_configuration = SM_REGISTERS[SM_REG_ADDR_CONFIGURATION_0];
	// Payloads loop.
	do {
		switch (SM_SIGFOX_UL_PAYLOAD_PATTERN[node_ul_payload -> node -> radio_transmission_count]) {
		case SM_SIGFOX_UL_PAYLOAD_TYPE_MONITORING:
			// Build registers list.
			reg_list.addr_list = (uint8_t*) SM_REG_LIST_SIGFOX_UL_PAYLOAD_MONITORING;
			reg_list.size = sizeof(SM_REG_LIST_SIGFOX_UL_PAYLOAD_MONITORING);
			// Perform measurements.
			status = XM_perform_measurements((node_ul_payload -> node -> address), &access_status);
			if (status != NODE_SUCCESS) goto errors;
			// Check write status.
			if (access_status.flags == 0) {
				// Read related registers.
				status = XM_read_registers((node_ul_payload -> node -> address), &reg_list, (XM_node_registers_t*) &SM_NODE_REGISTERS, &access_status);
				if (status != NODE_SUCCESS) goto errors;
			}
			// Build data payload.
			sigfox_ul_payload_monitoring.vmcu = DINFOX_read_field(SM_REGISTERS[COMMON_REG_ADDR_ANALOG_DATA_0], COMMON_REG_ANALOG_DATA_0_MASK_VMCU);
			sigfox_ul_payload_monitoring.tmcu = DINFOX_read_field(SM_REGISTERS[COMMON_REG_ADDR_ANALOG_DATA_0], COMMON_REG_ANALOG_DATA_0_MASK_TMCU);
			// Copy payload.
			for (idx=0 ; idx<SM_SIGFOX_UL_PAYLOAD_MONITORING_SIZE ; idx++) {
				(node_ul_payload -> ul_payload)[idx] = sigfox_ul_payload_monitoring.frame[idx];
			}
			(*(node_ul_payload -> size)) = SM_SIGFOX_UL_PAYLOAD_MONITORING_SIZE;
			break;
		case SM_SIGFOX_UL_PAYLOAD_TYPE_ELECTRICAL:
			// Check compilation flags.
			if (((reg_configuration & SM_REG_CONFIGURATION_0_MASK_AINF) == 0) && ((reg_configuration & SM_REG_CONFIGURATION_0_MASK_DIOF) == 0)) break;
			// Build registers list.
			reg_list.addr_list = (uint8_t*) SM_REG_LIST_SIGFOX_UL_PAYLOAD_ELECTRICAL;
			reg_list.size = sizeof(SM_REG_LIST_SIGFOX_UL_PAYLOAD_ELECTRICAL);
			// Perform measurements.
			status = XM_perform_measurements((node_ul_payload -> node -> address), &access_status);
			if (status != NODE_SUCCESS) goto errors;
			// Check write status.
			if (access_status.flags == 0) {
				// Read related registers.
				status = XM_read_registers((node_ul_payload -> node -> address), &reg_list, (XM_node_registers_t*) &SM_NODE_REGISTERS, &access_status);
				if (status != NODE_SUCCESS) goto errors;
			}
			// Build data payload.
			sigfox_ul_payload_electrical.ain0 = DINFOX_read_field(SM_REGISTERS[SM_REG_ADDR_ANALOG_DATA_1], SM_REG_ANALOG_DATA_1_MASK_VAIN0);
			sigfox_ul_payload_electrical.ain1 = DINFOX_read_field(SM_REGISTERS[SM_REG_ADDR_ANALOG_DATA_1], SM_REG_ANALOG_DATA_1_MASK_VAIN1);
			sigfox_ul_payload_electrical.ain2 = DINFOX_read_field(SM_REGISTERS[SM_REG_ADDR_ANALOG_DATA_2], SM_REG_ANALOG_DATA_2_MASK_VAIN2);
			sigfox_ul_payload_electrical.ain3 = DINFOX_read_field(SM_REGISTERS[SM_REG_ADDR_ANALOG_DATA_2], SM_REG_ANALOG_DATA_2_MASK_VAIN3);
			sigfox_ul_payload_electrical.dio0 = DINFOX_read_field(SM_REGISTERS[SM_REG_ADDR_DIGITAL_DATA], SM_REG_DIGITAL_DATA_MASK_DIO0);
			sigfox_ul_payload_electrical.dio1 = DINFOX_read_field(SM_REGISTERS[SM_REG_ADDR_DIGITAL_DATA], SM_REG_DIGITAL_DATA_MASK_DIO1);
			sigfox_ul_payload_electrical.dio2 = DINFOX_read_field(SM_REGISTERS[SM_REG_ADDR_DIGITAL_DATA], SM_REG_DIGITAL_DATA_MASK_DIO2);
			sigfox_ul_payload_electrical.dio3 = DINFOX_read_field(SM_REGISTERS[SM_REG_ADDR_DIGITAL_DATA], SM_REG_DIGITAL_DATA_MASK_DIO3);
			// Copy payload.
			for (idx=0 ; idx<SM_SIGFOX_UL_PAYLOAD_ELECTRICAL_SIZE ; idx++) {
				(node_ul_payload -> ul_payload)[idx] = sigfox_ul_payload_electrical.frame[idx];
			}
			(*(node_ul_payload -> size)) = SM_SIGFOX_UL_PAYLOAD_ELECTRICAL_SIZE;
			break;
		case SM_SIGFOX_UL_PAYLOAD_TYPE_SENSOR:
			// Check compilation flags.
			if ((reg_configuration & SM_REG_CONFIGURATION_0_MASK_DIGF) == 0) break;
			// Build registers list.
			reg_list.addr_list = (uint8_t*) SM_REG_LIST_SIGFOX_UL_PAYLOAD_SENSOR;
			reg_list.size = sizeof(SM_REG_LIST_SIGFOX_UL_PAYLOAD_SENSOR);
			// Perform measurements.
			status = XM_perform_measurements((node_ul_payload -> node -> address), &access_status);
			if (status != NODE_SUCCESS) goto errors;
			// Check write status.
			if (access_status.flags == 0) {
				// Read related registers.
				status = XM_read_registers((node_ul_payload -> node -> address), &reg_list, (XM_node_registers_t*) &SM_NODE_REGISTERS, &access_status);
				if (status != NODE_SUCCESS) goto errors;
			}
			// Build data payload.
			sigfox_ul_payload_sensor.tamb = DINFOX_read_field(SM_REGISTERS[SM_REG_ADDR_ANALOG_DATA_3], SM_REG_ANALOG_DATA_3_MASK_TAMB);
			sigfox_ul_payload_sensor.hamb = DINFOX_read_field(SM_REGISTERS[SM_REG_ADDR_ANALOG_DATA_3], SM_REG_ANALOG_DATA_3_MASK_HAMB);
			// Copy payload.
			for (idx=0 ; idx<SM_SIGFOX_UL_PAYLOAD_SENSOR_SIZE ; idx++) {
				(node_ul_payload -> ul_payload)[idx] = sigfox_ul_payload_sensor.frame[idx];
			}
			(*(node_ul_payload -> size)) = SM_SIGFOX_UL_PAYLOAD_SENSOR_SIZE;
			break;
		default:
			status = NODE_ERROR_SIGFOX_UL_PAYLOAD_TYPE;
			goto errors;
		}
		// Increment transmission count.
		(node_ul_payload -> node -> radio_transmission_count) = (uint8_t) (((node_ul_payload -> node -> radio_transmission_count) + 1) % (sizeof(SM_SIGFOX_UL_PAYLOAD_PATTERN)));
		// Exit in case of loop error.
		loop_count++;
		if (loop_count > SM_SIGFOX_UL_PAYLOAD_LOOP_MAX) goto errors;
	}
	while ((*(node_ul_payload -> size)) == 0);
errors:
	return status;
}
