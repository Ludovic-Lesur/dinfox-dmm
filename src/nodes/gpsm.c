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

#define GPSM_SIGFOX_PAYLOAD_MONITORING_SIZE		7

#define GPSM_SIGFOX_PAYLOAD_LOOP_MAX			10

/*** GPSM local structures ***/

typedef enum {
	GPSM_SIGFOX_PAYLOAD_TYPE_STARTUP = 0,
	GPSM_SIGFOX_PAYLOAD_TYPE_MONITORING,
	GPSM_SIGFOX_PAYLOAD_TYPE_ERROR_STACK,
	GPSM_SIGFOX_PAYLOAD_TYPE_LAST
} GPSM_sigfox_payload_type_t;

typedef union {
	uint8_t frame[GPSM_SIGFOX_PAYLOAD_MONITORING_SIZE];
	struct {
		unsigned vmcu : 16;
		unsigned tmcu : 8;
		unsigned vgps : 16;
		unsigned vant : 16;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} GPSM_sigfox_payload_monitoring_t;

/*** GPSM local global variables ***/

static uint32_t GPSM_REGISTERS[GPSM_REG_ADDR_LAST];

static const NODE_line_data_t GPSM_LINE_DATA[GPSM_LINE_DATA_INDEX_LAST - COMMON_LINE_DATA_INDEX_LAST] = {
	{"VGPS =", " V", STRING_FORMAT_DECIMAL, 0, GPSM_REG_ADDR_ANALOG_DATA_1, GPSM_REG_ANALOG_DATA_1_MASK_VGPS},
	{"VANT =", " V", STRING_FORMAT_DECIMAL, 0, GPSM_REG_ADDR_ANALOG_DATA_1, GPSM_REG_ANALOG_DATA_1_MASK_VANT},
};

static const uint32_t GPSM_REG_ERROR_VALUE[GPSM_REG_ADDR_LAST] = {
	COMMON_REG_ERROR_VALUE
	0x00000000,
	((DINFOX_VOLTAGE_ERROR_VALUE << 16) | (DINFOX_VOLTAGE_ERROR_VALUE << 0)),
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000
};

static const uint8_t GPSM_REG_LIST_SIGFOX_PAYLOAD_MONITORING[] = {
	COMMON_REG_ADDR_ANALOG_DATA_0,
	GPSM_REG_ADDR_ANALOG_DATA_1,
};

static const GPSM_sigfox_payload_type_t GPSM_SIGFOX_PAYLOAD_PATTERN[] = {
	GPSM_SIGFOX_PAYLOAD_TYPE_STARTUP,
	GPSM_SIGFOX_PAYLOAD_TYPE_MONITORING,
	GPSM_SIGFOX_PAYLOAD_TYPE_MONITORING,
	GPSM_SIGFOX_PAYLOAD_TYPE_MONITORING,
	GPSM_SIGFOX_PAYLOAD_TYPE_MONITORING,
	GPSM_SIGFOX_PAYLOAD_TYPE_MONITORING,
	GPSM_SIGFOX_PAYLOAD_TYPE_MONITORING,
	GPSM_SIGFOX_PAYLOAD_TYPE_MONITORING,
	GPSM_SIGFOX_PAYLOAD_TYPE_MONITORING,
	GPSM_SIGFOX_PAYLOAD_TYPE_MONITORING,
	GPSM_SIGFOX_PAYLOAD_TYPE_MONITORING,
	GPSM_SIGFOX_PAYLOAD_TYPE_ERROR_STACK
};

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
	XM_node_registers_t node_reg;
	STRING_status_t string_status = STRING_SUCCESS;
	uint32_t field_value = 0;
	char_t field_str[STRING_DIGIT_FUNCTION_SIZE];
	uint8_t str_data_idx = 0;
	uint8_t reg_addr = 0;
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
	// Build node registers structure.
	node_reg.value = (uint32_t*) GPSM_REGISTERS;
	node_reg.error = (uint32_t*) GPSM_REG_ERROR_VALUE;
	// Check common range.
	if ((line_data_read -> line_data_index) < COMMON_LINE_DATA_INDEX_LAST) {
		// Call common function.
		status = COMMON_read_line_data(line_data_read, &node_reg);
		if (status != NODE_SUCCESS) goto errors;
	}
	else {
		// Compute specific string data index and register address.
		str_data_idx = ((line_data_read -> line_data_index) - COMMON_LINE_DATA_INDEX_LAST);
		reg_addr = GPSM_LINE_DATA[str_data_idx].reg_addr;
		// Add data name.
		NODE_append_name_string((char_t*) GPSM_LINE_DATA[str_data_idx].name);
		buffer_size = 0;
		// Reset result to error.
		NODE_flush_string_value();
		NODE_append_value_string((char_t*) NODE_ERROR_STRING);
		// Update register.
		status = XM_read_register((line_data_read -> node_addr), reg_addr, GPSM_REG_ERROR_VALUE[reg_addr], &(GPSM_REGISTERS[reg_addr]), read_status);
		if ((status != NODE_SUCCESS) || ((read_status -> all) != 0)) goto errors;
		// Compute field.
		field_value = DINFOX_read_field(GPSM_REGISTERS[reg_addr], GPSM_LINE_DATA[str_data_idx].field_mask);
		// Check index.
		switch (line_data_read -> line_data_index) {
		case GPSM_LINE_DATA_INDEX_VGPS:
		case GPSM_LINE_DATA_INDEX_VANT:
			// Check error value.
			if (field_value != DINFOX_VOLTAGE_ERROR_VALUE) {
				// Convert to 5 digits string.
				string_status = STRING_value_to_5_digits_string(DINFOX_get_mv(field_value), (char_t*) field_str);
				STRING_check_status(NODE_ERROR_BASE_STRING);
				// Add string.
				NODE_flush_string_value();
				NODE_append_value_string(field_str);
				// Add unit.
				NODE_append_value_string((char_t*) GPSM_LINE_DATA[str_data_idx].unit);
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

/* UPDATE GPSM NODE SIGFOX UPLINK PAYLOAD.
 * @param ul_payload_update:	Pointer to the UL payload update structure.
 * @return status:				Function execution status.
 */
NODE_status_t GPSM_build_sigfox_ul_payload(NODE_ul_payload_update_t* ul_payload_update) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	NODE_access_status_t write_status;
	XM_node_registers_t node_reg;
	XM_registers_list_t reg_list;
	GPSM_sigfox_payload_monitoring_t sigfox_payload_monitoring;
	uint8_t idx = 0;
	uint32_t loop_count = 0;
	// Check parameters.
	if (ul_payload_update == NULL) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	if (((ul_payload_update -> ul_payload) == NULL) || ((ul_payload_update -> size) == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Build node registers structure.
	node_reg.value = (uint32_t*) GPSM_REGISTERS;
	node_reg.error = (uint32_t*) GPSM_REG_ERROR_VALUE;
	// Reset payload size.
	(*(ul_payload_update -> size)) = 0;
	// Main loop.
	do {
		// Check payload type.
		switch (GPSM_SIGFOX_PAYLOAD_PATTERN[ul_payload_update -> node -> radio_transmission_count]) {
		case GPSM_SIGFOX_PAYLOAD_TYPE_STARTUP:
			// Check flag.
			if ((ul_payload_update -> node -> startup_data_sent) == 0) {
				// Use common format.
				status = COMMON_build_sigfox_payload_startup(ul_payload_update, &node_reg);
				if (status != NODE_SUCCESS) goto errors;
				// Update flag.
				(ul_payload_update -> node -> startup_data_sent) = 1;
			}
			break;
		case GPSM_SIGFOX_PAYLOAD_TYPE_ERROR_STACK:
			// Use common format.
			status = COMMON_build_sigfox_payload_error_stack(ul_payload_update, &node_reg);
			if (status != NODE_SUCCESS) goto errors;
			break;
		case GPSM_SIGFOX_PAYLOAD_TYPE_MONITORING:
			// Build registers list.
			reg_list.addr_list = (uint8_t*) GPSM_REG_LIST_SIGFOX_PAYLOAD_MONITORING;
			reg_list.size = sizeof(GPSM_REG_LIST_SIGFOX_PAYLOAD_MONITORING);
			// Reset registers.
			status = XM_reset_registers(&reg_list, &node_reg);
			if (status != NODE_SUCCESS) goto errors;
			// Perform measurements.
			status = XM_perform_measurements((ul_payload_update -> node -> address), &write_status);
			if (status != NODE_SUCCESS) goto errors;
			// Check write status.
			if (write_status.all == 0) {
				// Read related registers.
				status = XM_read_registers((ul_payload_update -> node -> address), &reg_list, &node_reg);
				if (status != NODE_SUCCESS) goto errors;
			}
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
		default:
			status = NODE_ERROR_SIGFOX_PAYLOAD_TYPE;
			goto errors;
		}
		// Increment transmission count.
		(ul_payload_update -> node -> radio_transmission_count) = ((ul_payload_update -> node -> radio_transmission_count) + 1) % (sizeof(GPSM_SIGFOX_PAYLOAD_PATTERN));
		// Exit in case of loop error.
		loop_count++;
		if (loop_count > GPSM_SIGFOX_PAYLOAD_LOOP_MAX) {
			status = NODE_ERROR_SIGFOX_PAYLOAD_LOOP;
			goto errors;
		}
	}
	while ((*(ul_payload_update -> size)) == 0);
errors:
	return status;
}
