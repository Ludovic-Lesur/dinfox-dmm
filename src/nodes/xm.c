/*
 * xm.c
 *
 *  Created on: 14 jun. 2023
 *      Author: Ludo
 */

#include "xm.h"

#include "at_bus.h"
#include "common.h"
#include "common_reg.h"
#include "dinfox.h"
#include "dmm.h"
#include "node.h"
#include "node_common.h"

/*** XM functions ***/

/* WRITE BPSM DATA.
 * @param line_data_write:	Pointer to the data write structure.
 * @param node_line_data:	Pointer to the node line data.
 * @param read_status:		Pointer to the writing operation status.
 * @return status:			Function execution status.
 */
NODE_status_t XM_write_line_data(NODE_line_data_write_t* line_data_write, NODE_line_data_t* node_line_data, uint32_t* node_write_timeout, NODE_access_status_t* write_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	NODE_access_parameters_t write_params;
	uint8_t str_data_idx = 0;
	uint8_t reg_addr = 0;
	uint32_t reg_value = 0;
	uint32_t reg_mask = 0;
	uint32_t timeout_ms = 0;
	// Check address range.
	if ((line_data_write -> line_data_index) < COMMON_LINE_DATA_INDEX_LAST) {
		// Call common function.
		status = COMMON_write_line_data(line_data_write);
		if (status != NODE_SUCCESS) goto errors;
	}
	else {
		// Compute specific string data index.
		str_data_idx = ((line_data_write -> line_data_index) - COMMON_LINE_DATA_INDEX_LAST);
		// Compute parameters.
		reg_addr = node_line_data[str_data_idx].reg_addr;
		reg_mask = node_line_data[str_data_idx].field_mask;
		reg_value |= (line_data_write -> field_value) << DINFOX_get_field_offset(reg_mask);
		timeout_ms = node_write_timeout[reg_addr - COMMON_REG_ADDR_LAST];
		// Write parameters.
		write_params.node_addr = (line_data_write -> node_addr);
		write_params.reg_addr = reg_addr;
		write_params.reply_params.type = NODE_REPLY_TYPE_OK;
		write_params.reply_params.timeout_ms = timeout_ms;
		// Write register.
		status = AT_BUS_write_register(&write_params, reg_value, reg_mask, write_status);
		if (status != NODE_SUCCESS) goto errors;
	}
errors:
	return status;
}

/* TRIGGER MEASUREMENT ON NODE.
 * @param node_addr:	Node address.
 * @param write_status:	Pointer to the writing operation status.
 * @return status:		Function execution status.
 */
NODE_status_t XM_perform_measurements(NODE_address_t node_addr, NODE_access_status_t* write_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	NODE_access_parameters_t write_params;
	// Build parameters.
	write_params.node_addr = node_addr;
	write_params.reg_addr = COMMON_REG_ADDR_STATUS_CONTROL_0;
	write_params.reply_params.type = NODE_REPLY_TYPE_OK;
	write_params.reply_params.timeout_ms = COMMON_REG_WRITE_TIMEOUT_MS[COMMON_REG_ADDR_STATUS_CONTROL_0];
	// Write MTRG bit.
	if (node_addr == DINFOX_NODE_ADDRESS_DMM) {
		status = DMM_write_register(&write_params, COMMON_REG_STATUS_CONTROL_0_MASK_MTRG, COMMON_REG_STATUS_CONTROL_0_MASK_MTRG, write_status);
	}
	else {
		status = AT_BUS_write_register(&write_params, COMMON_REG_STATUS_CONTROL_0_MASK_MTRG, COMMON_REG_STATUS_CONTROL_0_MASK_MTRG, write_status);
	}
	return status;
}

/* READ NODE REGISTER.
 * @param node_addr:		Node address.
 * @param reg_addr:			Address of the register to update.
 * @param node_registers:	Pointer to the node registers.
 * @param read_status:		Pointer to the read operation status.
 * @return status:			Function execution status.
 */
NODE_status_t XM_read_register(NODE_address_t node_addr, uint8_t reg_addr, uint32_t* node_registers, NODE_access_status_t* read_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	NODE_access_parameters_t read_params;
	uint32_t reg_value = 0;
	// Read parameters.
	read_params.node_addr = node_addr;
	read_params.reg_addr = reg_addr;
	read_params.reply_params.type = NODE_REPLY_TYPE_VALUE;
	read_params.reply_params.timeout_ms = AT_BUS_DEFAULT_TIMEOUT_MS;
	// Read data.
	if (node_addr == DINFOX_NODE_ADDRESS_DMM) {
		status = DMM_read_register(&read_params, &reg_value, read_status);
	}
	else {
		status = AT_BUS_read_register(&read_params, &reg_value, read_status);
	}
	if (status != NODE_SUCCESS) goto errors;
	// Update local register.
	node_registers[reg_addr] = reg_value;
errors:
	return status;
}

/* UPDATE BPSM REGISTERS LIST.
 * @param node_addr:			Node address.
 * @param reg_addr_list:		List of register to update.
 * @param reg_addr_list_size:	Number of register(s) to update.
 * @param node_registers:		Pointer to the node registers.
 * @return status:				Function execution status.
 */
NODE_status_t XM_read_registers(NODE_address_t node_addr, uint8_t* reg_addr_list, uint8_t reg_addr_list_size, uint32_t* node_registers) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	NODE_access_status_t unused_read_status;
	uint8_t idx = 0;
	// Check parameters.
	if (reg_addr_list == NULL) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	if (reg_addr_list_size == 0) {
		status = NODE_ERROR_REGISTER_LIST_SIZE;
		goto errors;
	}
	// Read all registers.
	for (idx=0 ; idx<reg_addr_list_size ; idx++) {
		status = XM_read_register(node_addr, reg_addr_list[idx], node_registers, &unused_read_status);
		if (status != NODE_SUCCESS) goto errors;
	}
errors:
	return status;
}
