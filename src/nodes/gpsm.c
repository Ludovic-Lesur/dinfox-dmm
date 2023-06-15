/*
 * gpsm.c
 *
 *  Created on: 16 apr. 2023
 *      Author: Ludo
 */

#include "gpsm.h"

#include "at_bus.h"
#include "gpsm_reg.h"
#include "common.h"
#include "dinfox.h"
#include "node.h"
#include "string.h"
#include "xm.h"

/*** GPSM local macros ***/

#define GPSM_SIGFOX_PAYLOAD_MONITORING_SIZE		3
#define GPSM_SIGFOX_PAYLOAD_ELECTRICAL_SIZE		7

/*** GPSM local global variables ***/

static uint32_t GPSM_REGISTERS[GPSM_REG_ADDR_LAST];

static const NODE_line_data_t GPSM_LINE_DATA[GPSM_LINE_DATA_INDEX_LAST - COMMON_LINE_DATA_INDEX_LAST] = {
	{GPSM_REG_ADDR_ANALOG_DATA_1, GPSM_REG_ANALOG_DATA_1_MASK_VGPS, "VGPS =", STRING_FORMAT_DECIMAL, 0, " V"},
	{GPSM_REG_ADDR_ANALOG_DATA_1, GPSM_REG_ANALOG_DATA_1_MASK_VANT, "VANT =", STRING_FORMAT_DECIMAL, 0, " V"},
};

static const uint8_t GPSM_REG_ADDR_LIST_SIGFOX_PAYLOAD_MONITORING[] = {
	COMMON_REG_ADDR_ANALOG_DATA_0,
	GPSM_REG_ADDR_ANALOG_DATA_1,
};

/*** GPSM local structures ***/

typedef union {
	uint8_t frame[GPSM_SIGFOX_PAYLOAD_MONITORING_SIZE];
	struct {
		unsigned vmcu : 16;
		unsigned tmcu : 8;
		unsigned vgps : 16;
		unsigned vant : 16;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} GPSM_sigfox_payload_monitoring_t;

/*** GPSM functions ***/

/* WRITE GPSM DATA.
 * @param line_data_write:	Pointer to the data write structure.
 * @param read_status:		Pointer to the writing operation status.
 * @return status:			Function execution status.
 */
NODE_status_t GPSM_write_line_data(NODE_line_data_write_t* line_data_write, NODE_access_status_t* write_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	// Call common function with local data.
	status = XM_write_line_data(line_data_write, (NODE_line_data_t*) GPSM_LINE_DATA, (uint32_t*) GPSM_REG_WRITE_TIMEOUT_MS, write_status);
	return status;
}

/* READ GPSM DATA.
 * @param line_data_read:	Pointer to the data read structure.
 * @param read_status:		Pointer to the reading operation status.
 * @return status:			Function execution status.
 */
NODE_status_t GPSM_read_line_data(NODE_line_data_read_t* line_data_read, NODE_access_status_t* read_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	uint32_t field_value = 0;
	char_t field_str[STRING_DIGIT_FUNCTION_SIZE];
	uint8_t str_data_idx = 0;
	uint8_t buffer_size = 0;
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
	if ((line_data_read -> line_data_index) >= GPSM_LINE_DATA_INDEX_LAST) {
		status = NODE_ERROR_LINE_DATA_INDEX;
		goto errors;
	}
	// Check common range.
	if ((line_data_read -> line_data_index) < COMMON_LINE_DATA_INDEX_LAST) {
		// Call common function.
		status = COMMON_read_line_data(line_data_read, (uint32_t*) GPSM_REGISTERS);
		if (status != NODE_SUCCESS) goto errors;
	}
	else {
		// Compute specific string data index.
		str_data_idx = ((line_data_read -> line_data_index) - COMMON_LINE_DATA_INDEX_LAST);
		// Update register.
		status = XM_read_register((line_data_read -> node_addr), GPSM_LINE_DATA[str_data_idx].reg_addr, (uint32_t*) GPSM_REGISTERS, read_status);
		if (status != NODE_SUCCESS) goto errors;
		// Compute field.
		field_value = DINFOX_read_field(GPSM_REGISTERS[GPSM_LINE_DATA[str_data_idx].reg_addr], GPSM_LINE_DATA[str_data_idx].field_mask);
		// Add data name.
		NODE_append_name_string((char_t*) GPSM_LINE_DATA[str_data_idx].name);
		buffer_size = 0;
		// Add data value.
		if ((read_status -> all) == 0) {
			// Check index.
			switch (line_data_read -> line_data_index) {
			case GPSM_LINE_DATA_INDEX_VGPS:
			case GPSM_LINE_DATA_INDEX_VANT:
				// Convert to 5 digits string.
				string_status = STRING_value_to_5_digits_string(DINFOX_get_mv(field_value), (char_t*) field_str);
				STRING_status_check(NODE_ERROR_BASE_STRING);
				// Add string.
				NODE_append_value_string(field_str);
				break;
			default:
				NODE_append_value_int32(field_value, STRING_FORMAT_HEXADECIMAL, 1);
				break;
			}
			// Add unit.
			NODE_append_value_string((char_t*) GPSM_LINE_DATA[str_data_idx].unit);
		}
		else {
			NODE_flush_string_value();
			NODE_append_value_string((char_t*) NODE_ERROR_STRING);
		}
	}
errors:
	return status;
}

/* UPDATE GPSM NODE SIGFOX UPLINK PAYLOAD.
 * @param ul_payload_update:	Pointer to the UL payload update structure.
 * @return status:				Function execution status.
 */
NODE_status_t GPSM_build_sigfox_ul_payload(NODE_ul_payload_update_t* ul_payload_update) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	NODE_access_status_t unused_write_access;
	GPSM_sigfox_payload_monitoring_t sigfox_payload_monitoring;
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
		status = COMMON_build_sigfox_payload_startup(ul_payload_update, (uint32_t*) GPSM_REGISTERS);
		if (status != NODE_SUCCESS) goto errors;
		break;
	case NODE_SIGFOX_PAYLOAD_TYPE_MONITORING:
		// Perform measurements.
		status = XM_perform_measurements((ul_payload_update -> node_addr), &unused_write_access);
		if (status != NODE_SUCCESS) goto errors;
		// Read related registers.
		status = XM_read_registers((ul_payload_update -> node_addr), (uint8_t*) GPSM_REG_ADDR_LIST_SIGFOX_PAYLOAD_MONITORING, sizeof(GPSM_REG_ADDR_LIST_SIGFOX_PAYLOAD_MONITORING), (uint32_t*) GPSM_REGISTERS);
		if (status != NODE_SUCCESS) goto errors;
		// Build monitoring payload.
		sigfox_payload_monitoring.vmcu = DINFOX_read_field(GPSM_REGISTERS[COMMON_REG_ADDR_ANALOG_DATA_0], COMMON_REG_ANALOG_DATA_0_MASK_VMCU);
		sigfox_payload_monitoring.tmcu = DINFOX_read_field(GPSM_REGISTERS[COMMON_REG_ADDR_ANALOG_DATA_0], COMMON_REG_ANALOG_DATA_0_MASK_TMCU);
		sigfox_payload_monitoring.vgps = DINFOX_read_field(GPSM_REGISTERS[GPSM_REG_ADDR_ANALOG_DATA_1], GPSM_REG_ANALOG_DATA_1_MASK_VGPS);
		sigfox_payload_monitoring.vant = DINFOX_read_field(GPSM_REGISTERS[GPSM_REG_ADDR_ANALOG_DATA_1], GPSM_REG_ANALOG_DATA_1_MASK_VANT);
		// Copy payload.
		for (idx=0 ; idx<GPSM_SIGFOX_PAYLOAD_MONITORING_SIZE ; idx++) {
			(ul_payload_update -> ul_payload)[idx] = sigfox_payload_monitoring.frame[idx];
		}
		(*(ul_payload_update -> size)) = GPSM_SIGFOX_PAYLOAD_MONITORING_SIZE;
		break;
	case NODE_SIGFOX_PAYLOAD_TYPE_DATA:
		// No data frame.
		(*(ul_payload_update -> size)) = 0;
		status = NODE_ERROR_SIGFOX_PAYLOAD_EMPTY;
		goto errors;
	default:
		status = NODE_ERROR_SIGFOX_PAYLOAD_TYPE;
		goto errors;
	}
errors:
	return status;
}
