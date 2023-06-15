/*
 * uhfm.c
 *
 *  Created on: 26 feb. 2023
 *      Author: Ludo
 */

#include "uhfm.h"

#include "at_bus.h"
#include "uhfm_reg.h"
#include "common.h"
#include "dinfox.h"
#include "node.h"
#include "string.h"
#include "xm.h"

/*** UHFM local macros ***/

#define UHFM_SIGFOX_PAYLOAD_MONITORING_SIZE		7

/*** UHFM local global variables ***/

static uint32_t UHFM_REGISTERS[UHFM_REG_ADDR_LAST];

static const NODE_line_data_t UHFM_LINE_DATA[UHFM_LINE_DATA_INDEX_LAST - COMMON_LINE_DATA_INDEX_LAST] = {
	{UHFM_REG_ADDR_ANALOG_DATA_1, UHFM_REG_ANALOG_DATA_1_MASK_VRF_TX, "VRF TX =", STRING_FORMAT_DECIMAL, 0, " V"},
	{UHFM_REG_ADDR_ANALOG_DATA_1, UHFM_REG_ANALOG_DATA_1_MASK_VRF_TX, "VRF RX =", STRING_FORMAT_DECIMAL, 0, " V"}
};

static const uint8_t UHFM_REG_ADDR_LIST_SIGFOX_PAYLOAD_MONITORING[] = {
	COMMON_REG_ADDR_ANALOG_DATA_0
};

/*** UHFM local structures ***/

typedef union {
	uint8_t frame[UHFM_SIGFOX_PAYLOAD_MONITORING_SIZE];
	struct {
		unsigned vmcu : 16;
		unsigned tmcu : 8;
		unsigned vrf_tx : 16;
		unsigned vrf_rx : 16;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} UHFM_sigfox_payload_monitoring_t;

/*** UHFM functions ***/

/* WRITE UHFM DATA.
 * @param line_data_write:	Pointer to the data write structure.
 * @param read_status:		Pointer to the writing operation status.
 * @return status:			Function execution status.
 */
NODE_status_t UHFM_write_line_data(NODE_line_data_write_t* line_data_write, NODE_access_status_t* write_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	// Call common function with local data.
	status = XM_write_line_data(line_data_write, (NODE_line_data_t*) UHFM_LINE_DATA, (uint32_t*) UHFM_REG_WRITE_TIMEOUT_MS, write_status);
	return status;
}

/* READ UHFM DATA.
 * @param line_data_read:	Pointer to the data read structure.
 * @param read_status:		Pointer to the reading operation status.
 * @return status:			Function execution status.
 */
NODE_status_t UHFM_read_line_data(NODE_line_data_read_t* line_data_read, NODE_access_status_t* read_status) {
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
	if ((line_data_read -> line_data_index) >= UHFM_LINE_DATA_INDEX_LAST) {
		status = NODE_ERROR_LINE_DATA_INDEX;
		goto errors;
	}
	// Check common range.
	if ((line_data_read -> line_data_index) < COMMON_LINE_DATA_INDEX_LAST) {
		// Call common function.
		status = COMMON_read_line_data(line_data_read, (uint32_t*) UHFM_REGISTERS);
		if (status != NODE_SUCCESS) goto errors;
	}
	else {
		// Compute specific string data index.
		str_data_idx = ((line_data_read -> line_data_index) - COMMON_LINE_DATA_INDEX_LAST);
		// Update register.
		status = XM_read_register((line_data_read -> node_addr), UHFM_LINE_DATA[str_data_idx].reg_addr, (uint32_t*) UHFM_REGISTERS, read_status);
		if (status != NODE_SUCCESS) goto errors;
		// Compute field.
		field_value = DINFOX_read_field(UHFM_REGISTERS[UHFM_LINE_DATA[str_data_idx].reg_addr], UHFM_LINE_DATA[str_data_idx].field_mask);
		// Add data name.
		NODE_append_name_string((char_t*) UHFM_LINE_DATA[str_data_idx].name);
		buffer_size = 0;
		// Add data value.
		if ((read_status -> all) == 0) {
			// Check index.
			switch (line_data_read -> line_data_index) {
			case UHFM_LINE_DATA_INDEX_VRF_TX:
			case UHFM_LINE_DATA_INDEX_VRF_RX:
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
			NODE_append_value_string((char_t*) UHFM_LINE_DATA[str_data_idx].unit);
		}
		else {
			NODE_flush_string_value();
			NODE_append_value_string((char_t*) NODE_ERROR_STRING);
		}
	}
errors:
	return status;
}

/* UPDATE UHFM NODE SIGFOX UPLINK PAYLOAD.
 * @param ul_payload_update:	Pointer to the UL payload update structure.
 * @return status:				Function execution status.
 */
NODE_status_t UHFM_build_sigfox_ul_payload(NODE_ul_payload_update_t* ul_payload_update) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	NODE_access_status_t unused_write_access;
	UHFM_sigfox_payload_monitoring_t sigfox_payload_monitoring;
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
		status = COMMON_build_sigfox_payload_startup(ul_payload_update, (uint32_t*) UHFM_REGISTERS);
		if (status != NODE_SUCCESS) goto errors;
		break;
	case NODE_SIGFOX_PAYLOAD_TYPE_MONITORING:
		// Perform measurements.
		status = XM_perform_measurements((ul_payload_update -> node_addr), &unused_write_access);
		if (status != NODE_SUCCESS) goto errors;
		// Read related registers.
		status = XM_read_registers((ul_payload_update -> node_addr), (uint8_t*) UHFM_REG_ADDR_LIST_SIGFOX_PAYLOAD_MONITORING, sizeof(UHFM_REG_ADDR_LIST_SIGFOX_PAYLOAD_MONITORING), (uint32_t*) UHFM_REGISTERS);
		if (status != NODE_SUCCESS) goto errors;
		// Build monitoring payload.
		sigfox_payload_monitoring.vmcu = DINFOX_read_field(UHFM_REGISTERS[COMMON_REG_ADDR_ANALOG_DATA_0], COMMON_REG_ANALOG_DATA_0_MASK_VMCU);
		sigfox_payload_monitoring.tmcu = DINFOX_read_field(UHFM_REGISTERS[COMMON_REG_ADDR_ANALOG_DATA_0], COMMON_REG_ANALOG_DATA_0_MASK_TMCU);
		sigfox_payload_monitoring.vrf_tx = DINFOX_read_field(UHFM_REGISTERS[UHFM_REG_ADDR_ANALOG_DATA_1], UHFM_REG_ANALOG_DATA_1_MASK_VRF_TX);
		sigfox_payload_monitoring.vrf_rx = DINFOX_read_field(UHFM_REGISTERS[UHFM_REG_ADDR_ANALOG_DATA_1], UHFM_REG_ANALOG_DATA_1_MASK_VRF_RX);
		// Copy payload.
		for (idx=0 ; idx<UHFM_SIGFOX_PAYLOAD_MONITORING_SIZE ; idx++) {
			(ul_payload_update -> ul_payload)[idx] = sigfox_payload_monitoring.frame[idx];
		}
		(*(ul_payload_update -> size)) = UHFM_SIGFOX_PAYLOAD_MONITORING_SIZE;
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

/* SEND SIGFOX MESSAGE WITH UHFM MODULE.
 * @param node_address:		Address of the UHFM node to use.
 * @param sigfox_message:	Pointer to the Sigfox message structure.
 * @param send_status:		Pointer to the sending status.
 * @return status:			Function execution status.
 */
NODE_status_t UHFM_send_sigfox_message(NODE_address_t node_addr, UHFM_sigfox_message_t* sigfox_message, NODE_access_status_t* send_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	NODE_access_parameters_t write_params;
	uint32_t reg_value = 0;
	uint32_t reg_mask = 0;
	uint8_t reg_offset = 0;
	uint8_t idx = 0;
	// Common writing parameters.
	write_params.node_addr = node_addr;
	write_params.reply_params.type = NODE_REPLY_TYPE_OK;
	write_params.reply_params.timeout_ms = AT_BUS_DEFAULT_TIMEOUT_MS;
	// Configuration register 2.
	reg_value |= (0x03 << DINFOX_get_field_offset(UHFM_REG_SIGFOX_EP_CONFIGURATION_2_MASK_MSGT));
	reg_mask |= UHFM_REG_SIGFOX_EP_CONFIGURATION_2_MASK_MSGT;
	reg_value |= ((sigfox_message -> bidirectional_flag) << DINFOX_get_field_offset(UHFM_REG_SIGFOX_EP_CONFIGURATION_2_MASK_BF));
	reg_mask |= UHFM_REG_SIGFOX_EP_CONFIGURATION_2_MASK_BF;
	reg_value |= ((sigfox_message -> ul_payload_size) << DINFOX_get_field_offset(UHFM_REG_SIGFOX_EP_CONFIGURATION_2_MASK_UL_PAYLOAD_SIZE));
	reg_mask |= UHFM_REG_SIGFOX_EP_CONFIGURATION_2_MASK_UL_PAYLOAD_SIZE;
	// Write register.
	write_params.reg_addr = UHFM_REG_ADDR_SIGFOX_EP_CONFIGURATION_2;
	status = AT_BUS_write_register(&write_params, reg_value, reg_mask, send_status);
	if (status != NODE_SUCCESS) goto errors;
	// UL payload.
	reg_value = 0;
	for (idx=0 ; idx<(sigfox_message -> ul_payload_size) ; idx++) {
		// Build register.
		reg_value |= (((sigfox_message -> ul_payload)[idx]) << (8 * (idx % 4)));
		// Check index.
		if ((((idx + 1) % 4) == 0) || (idx == ((sigfox_message -> ul_payload_size) - 1))) {
			// Write register.
			write_params.reg_addr = UHFM_REG_ADDR_SIGFOX_UL_PAYLOAD_0 + reg_offset;
			status = AT_BUS_write_register(&write_params, reg_value, DINFOX_REG_MASK_ALL, send_status);
			if (status != NODE_SUCCESS) goto errors;
			// Go to next register and reset value.
			reg_offset++;
			reg_value = 0;
		}
	}
	// Set proper timeout.
	write_params.reg_addr = UHFM_REG_ADDR_STATUS_CONTROL_1;
	write_params.reply_params.timeout_ms = ((sigfox_message -> bidirectional_flag) == 0) ? 10000 : 60000;
	// Send message.
	status = AT_BUS_write_register(&write_params, UHFM_REG_STATUS_CONTROL_1_MASK_STRG, UHFM_REG_STATUS_CONTROL_1_MASK_STRG, send_status);
errors:
	return status;
}

/* READ DOWNLINK PAYLOAD.
 * @param node_address:	Address of the UHFM node to use.
 * @param dl_payload:	Byte array that will contain the DL payload.
 * @param send_status:	Pointer to the read status.
 * @return status:		Function execution status.
 */
NODE_status_t UHFM_get_dl_payload(NODE_address_t node_addr, uint8_t* dl_payload, NODE_access_status_t* read_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	NODE_access_parameters_t read_params;
	UHFM_message_status_t message_status;
	uint32_t reg_value = 0;
	uint8_t reg_offset = 0;
	uint8_t idx = 0;
	// Reading parameters.
	read_params.node_addr = node_addr;
	read_params.reply_params.type = NODE_REPLY_TYPE_VALUE;
	read_params.reply_params.timeout_ms = AT_BUS_DEFAULT_TIMEOUT_MS;
	// Read message status.
	read_params.reg_addr = UHFM_REG_ADDR_STATUS_CONTROL_1;
	status = AT_BUS_read_register(&read_params, &reg_value, read_status);
	if (status != NODE_SUCCESS) goto errors;
	// Compute message status.
	message_status.all = DINFOX_read_field(reg_value, UHFM_REG_STATUS_CONTROL_1_MASK_MESSAGE_STATUS);
	// Check DL flag.
	if (message_status.dl_frame == 0) {
		status = NODE_ERROR_DOWNLINK_PAYLOAD_NOT_AVAILABLE;
		goto errors;
	}
	// Byte loop.
	for (idx=0 ; idx<UHFM_SIGFOX_DL_PAYLOAD_SIZE ; idx++) {
		// Check index.
		if ((idx % 4) == 0) {
			// Read register.
			read_params.reg_addr = UHFM_REG_ADDR_SIGFOX_DL_PAYLOAD_0 + reg_offset;
			status = AT_BUS_read_register(&read_params, &reg_value, read_status);
			if (status != NODE_SUCCESS) goto errors;
			// Go to next register and reset value.
			reg_offset++;
			reg_value = 0;
		}
		// Convert to byte array.
		dl_payload[idx] = (uint8_t) ((reg_value >> (8 * (idx % 4))) & 0xFF);
	}
errors:
	return status;
}
