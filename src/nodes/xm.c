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
#include "r4s8cr.h"

/*** XM functions ***/

/*******************************************************************/
NODE_status_t XM_write_register(NODE_address_t node_addr, uint8_t reg_addr, uint32_t reg_value, uint32_t reg_mask, uint32_t timeout_ms, NODE_access_status_t* write_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	NODE_access_parameters_t write_params;
	// Write parameters.
	write_params.node_addr = node_addr;
	write_params.reg_addr = reg_addr;
	write_params.reply_params.type = NODE_REPLY_TYPE_OK;
	write_params.reply_params.timeout_ms = timeout_ms;
	// Write register.
	if (node_addr == DINFOX_NODE_ADDRESS_DMM) {
		status = DMM_write_register(&write_params, reg_value, reg_mask, write_status);
	}
	else {
		if ((node_addr >= DINFOX_NODE_ADDRESS_R4S8CR_START) && (node_addr < (DINFOX_NODE_ADDRESS_R4S8CR_START + DINFOX_NODE_ADDRESS_RANGE_R4S8CR))) {
			status = R4S8CR_write_register(&write_params, reg_value, reg_mask, write_status);
		}
		else {
			status = AT_BUS_write_register(&write_params, reg_value, reg_mask, write_status);
		}
	}
	return status;
}

/*******************************************************************/
NODE_status_t XM_read_register(NODE_address_t node_addr, uint8_t reg_addr, uint32_t reg_error_value, uint32_t* reg_value, NODE_access_status_t* read_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	NODE_access_parameters_t read_params;
	uint32_t local_reg_value = 0;
	// Read parameters.
	read_params.node_addr = node_addr;
	read_params.reg_addr = reg_addr;
	read_params.reply_params.type = NODE_REPLY_TYPE_VALUE;
	read_params.reply_params.timeout_ms = AT_BUS_DEFAULT_TIMEOUT_MS;
	// Reset value.
	(*reg_value) = reg_error_value;
	// Read data.
	if (node_addr == DINFOX_NODE_ADDRESS_DMM) {
		status = DMM_read_register(&read_params, &local_reg_value, read_status);
	}
	else {
		if ((node_addr >= DINFOX_NODE_ADDRESS_R4S8CR_START) && (node_addr < (DINFOX_NODE_ADDRESS_R4S8CR_START + DINFOX_NODE_ADDRESS_RANGE_R4S8CR))) {
			status = R4S8CR_read_register(&read_params, &local_reg_value, read_status);
		}
		else {
			status = AT_BUS_read_register(&read_params, &local_reg_value, read_status);
		}
	}
	if ((status != NODE_SUCCESS) || ((read_status -> all) != 0)) goto errors;
	// Update value.
	(*reg_value) = local_reg_value;
errors:
	return status;
}

/*******************************************************************/
NODE_status_t XM_read_registers(NODE_address_t node_addr, XM_registers_list_t* reg_list, XM_node_registers_t* node_reg) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	NODE_access_status_t unused_read_status;
	uint8_t reg_addr = 0;
	uint8_t idx = 0;
	// Check parameters.
	if ((reg_list == NULL) || (node_reg == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	if (((reg_list -> addr_list) == NULL) || ((node_reg -> value) == NULL) || ((node_reg -> error) == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	if ((reg_list -> size) == 0) {
		status = NODE_ERROR_REGISTER_LIST_SIZE;
		goto errors;
	}
	// Read all registers.
	for (idx=0 ; idx<(reg_list -> size) ; idx++) {
		// Read register.
		// Note: status is not checked is order fill all registers with their error value.
		reg_addr = (reg_list -> addr_list)[idx];
		XM_read_register(node_addr, reg_addr, (node_reg -> error)[reg_addr], &((node_reg -> value)[reg_addr]), &unused_read_status);
	}
errors:
	return status;
}


/*******************************************************************/
NODE_status_t XM_reset_registers(XM_registers_list_t* reg_list, XM_node_registers_t* node_reg) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	uint8_t idx = 0;
	uint8_t reg_addr = 0;
	// Check parameters.
	if ((reg_list == NULL) || (node_reg == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	if (((reg_list -> addr_list) == NULL) || ((node_reg -> value) == NULL) || ((node_reg -> error) == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	if ((reg_list -> size) == 0) {
		status = NODE_ERROR_REGISTER_LIST_SIZE;
		goto errors;
	}
	// Read all registers.
	for (idx=0 ; idx<(reg_list -> size) ; idx++) {
		// Reset to error value.
		reg_addr = (reg_list -> addr_list)[idx];
		(node_reg -> value)[reg_addr] = (node_reg -> error)[reg_addr];
	}
errors:
	return status;
}

/*******************************************************************/
NODE_status_t XM_write_line_data(NODE_line_data_write_t* line_data_write, NODE_line_data_t* node_line_data, uint32_t* node_write_timeout, NODE_access_status_t* write_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	uint8_t reg_addr = 0;
	uint32_t reg_value = 0;
	uint32_t reg_mask = 0;
	uint32_t timeout_ms = 0;
	// Compute parameters.
	reg_addr = node_line_data[(line_data_write -> line_data_index)].write_reg_addr;
	DINFOX_write_field(&reg_value, &reg_mask, (line_data_write -> field_value), node_line_data[(line_data_write -> line_data_index)].write_field_mask);
	timeout_ms = node_write_timeout[reg_addr - COMMON_REG_ADDR_LAST];
	// Write register.
	status = XM_write_register((line_data_write -> node_addr), reg_addr, reg_value, reg_mask, timeout_ms, write_status);
	return status;
}

/*******************************************************************/
NODE_status_t XM_perform_measurements(NODE_address_t node_addr, NODE_access_status_t* write_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	// Write MTRG bit.
	status = XM_write_register(node_addr, COMMON_REG_ADDR_CONTROL_0, COMMON_REG_CONTROL_0_MASK_MTRG, COMMON_REG_CONTROL_0_MASK_MTRG, COMMON_REG_WRITE_TIMEOUT_MS[COMMON_REG_ADDR_CONTROL_0], write_status);
	return status;
}
