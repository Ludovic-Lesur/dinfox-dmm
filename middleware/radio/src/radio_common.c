/*
 * radio_common.c
 *
 *  Created on: 26 jan 2023
 *      Author: Ludo
 */

#include "radio_common.h"

#include "common_registers.h"
#include "node.h"
#include "radio.h"
#include "rcc_reg.h"
#include "string.h"
#include "swreg.h"
#include "types.h"
#include "una.h"
#include "version.h"

/*** RADIO COMMON local macros ***/

#define RADIO_COMMON_UL_PAYLOAD_ACTION_LOG_SIZE     8
#define RADIO_COMMON_UL_PAYLOAD_STARTUP_SIZE		8
#define RADIO_COMMON_UL_PAYLOAD_ERROR_STACK_SIZE	10

/*** RADIO COMMON local structures ***/

/*******************************************************************/
typedef union {
	uint8_t frame[RADIO_COMMON_UL_PAYLOAD_ACTION_LOG_SIZE];
	struct {
		unsigned marker : 4;
		unsigned downlink_hash : 12;
		unsigned reg_addr : 8;
		unsigned reg_value : 32;
		unsigned node_access_status : 8;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} RADIO_COMMON_ul_payload_action_log_t;

/*******************************************************************/
typedef union {
	uint8_t frame[RADIO_COMMON_UL_PAYLOAD_STARTUP_SIZE];
	struct {
		unsigned reset_reason : 8;
		unsigned major_version : 8;
		unsigned minor_version : 8;
		unsigned commit_index : 8;
		unsigned commit_id : 28;
		unsigned dirty_flag : 4;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} RADIO_COMMON_ul_payload_startup_t;

/*** COMMON local global variables ***/

static const uint8_t RADIO_COMMON_REGISTERS_STARTUP[] = {
	COMMON_REGISTER_ADDRESS_SW_VERSION_0,
	COMMON_REGISTER_ADDRESS_SW_VERSION_1,
	COMMON_REGISTER_ADDRESS_STATUS_0
};

/*** RADIO COMMON local functions ***/

/*******************************************************************/
static RADIO_status_t _RADIO_COMMON_build_ul_node_payload_startup(RADIO_ul_node_payload_t* node_payload, uint32_t* node_registers) {
	// Local variables.
	RADIO_status_t status = RADIO_SUCCESS;
	NODE_status_t node_status = NODE_SUCCESS;
	UNA_access_status_t access_status;
	RADIO_COMMON_ul_payload_startup_t ul_payload_startup;
	uint8_t idx = 0;
	// Reset payload size.
	node_payload->payload_size = 0;
	// Read related registers.
	node_status = NODE_read_registers((node_payload->node), (uint8_t*) RADIO_COMMON_REGISTERS_STARTUP, sizeof(RADIO_COMMON_REGISTERS_STARTUP), node_registers, &access_status);
	NODE_exit_error(RADIO_ERROR_BASE_NODE);
	// Check read status.
	if (access_status.flags != 0) goto errors;
	// Build data payload.
	ul_payload_startup.reset_reason = SWREG_read_field(node_registers[COMMON_REGISTER_ADDRESS_STATUS_0], COMMON_REGISTER_STATUS_0_MASK_RESET_FLAGS);
	ul_payload_startup.major_version = SWREG_read_field(node_registers[COMMON_REGISTER_ADDRESS_SW_VERSION_0], COMMON_REGISTER_SW_VERSION_0_MASK_MAJOR);
	ul_payload_startup.minor_version = SWREG_read_field(node_registers[COMMON_REGISTER_ADDRESS_SW_VERSION_0], COMMON_REGISTER_SW_VERSION_0_MASK_MINOR);
	ul_payload_startup.commit_index = SWREG_read_field(node_registers[COMMON_REGISTER_ADDRESS_SW_VERSION_0], COMMON_REGISTER_SW_VERSION_0_MASK_COMMIT_INDEX);
	ul_payload_startup.commit_id = SWREG_read_field(node_registers[COMMON_REGISTER_ADDRESS_SW_VERSION_1], COMMON_REGISTER_SW_VERSION_1_MASK_COMMIT_ID);
	ul_payload_startup.dirty_flag = SWREG_read_field(node_registers[COMMON_REGISTER_ADDRESS_SW_VERSION_0], COMMON_REGISTER_SW_VERSION_0_MASK_DTYF);
	// Copy payload.
	for (idx=0 ; idx<RADIO_COMMON_UL_PAYLOAD_STARTUP_SIZE ; idx++) {
		(node_payload->payload)[idx] = ul_payload_startup.frame[idx];
	}
	node_payload->payload_size = RADIO_COMMON_UL_PAYLOAD_STARTUP_SIZE;
errors:
	return status;
}

/*******************************************************************/
static RADIO_status_t _RADIO_COMMON_build_ul_node_payload_error_stack(RADIO_ul_node_payload_t* node_payload, uint32_t* node_registers) {
	// Local variables.
	RADIO_status_t status = RADIO_SUCCESS;
	NODE_status_t node_status = NODE_SUCCESS;
	UNA_access_status_t access_status;
	uint32_t reg_value = 0;
	uint8_t idx = 0;
	// Reset payload size.
	node_payload->payload_size = 0;
	// Read error stack register.
	for (idx=0 ; idx<(RADIO_COMMON_UL_PAYLOAD_ERROR_STACK_SIZE >> 1) ; idx++) {
		// Read error stack register.
	    node_status = NODE_read_register((node_payload->node), COMMON_REGISTER_ADDRESS_ERROR_STACK, &(node_registers[COMMON_REGISTER_ADDRESS_ERROR_STACK]), &access_status);
	    NODE_exit_error(RADIO_ERROR_BASE_NODE);
	    // Check read status.
        if (access_status.flags != 0) goto errors;
		// Update local value.
		reg_value = node_registers[COMMON_REGISTER_ADDRESS_ERROR_STACK];
		// If the first error is zero, the stack is empty, no frame has to be sent.
		if ((idx == 0) && ((reg_value & COMMON_REGISTER_ERROR_STACK_MASK_ERROR) == 0)) goto errors;
		// Fill data.
		(node_payload->payload)[(idx << 1) + 0] = (uint8_t) ((reg_value >> 8) & 0x000000FF);
		(node_payload->payload)[(idx << 1) + 1] = (uint8_t) ((reg_value >> 0) & 0x000000FF);
	}
	node_payload->payload_size = RADIO_COMMON_UL_PAYLOAD_ERROR_STACK_SIZE;
errors:
	return status;
}

/*** RADIO COMMON functions ***/

/*******************************************************************/
RADIO_status_t RADIO_COMMON_check_event_driven_payloads(RADIO_ul_node_payload_t* node_payload, uint32_t* node_registers) {
	// Local variables.
	RADIO_status_t status = RADIO_SUCCESS;
	NODE_status_t node_status = NODE_SUCCESS;
	uint32_t reg_status_0 = 0;
	UNA_access_status_t access_status;
	// Check parameters.
	if ((node_payload == NULL) || (node_registers == NULL)) {
		status = RADIO_ERROR_NULL_PARAMETER;
		goto errors;
	}
	if (((node_payload->node) == NULL) || ((node_payload->payload) == NULL)) {
		status = RADIO_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Reset payload size.
	node_payload->payload_size = 0;
	// Read status register.
	node_status = NODE_read_register((node_payload->node), COMMON_REGISTER_ADDRESS_STATUS_0, &(node_registers[COMMON_REGISTER_ADDRESS_STATUS_0]), &access_status);
	NODE_exit_error(RADIO_ERROR_BASE_NODE);
	// Check read status.
    if (access_status.flags != 0) goto errors;
	// Update local value.
	reg_status_0 = node_registers[COMMON_REGISTER_ADDRESS_STATUS_0];
	// Read boot flag.
	if (SWREG_read_field(reg_status_0, COMMON_REGISTER_STATUS_0_MASK_BF) != 0) {
		// Compute startup payload.
		status = _RADIO_COMMON_build_ul_node_payload_startup(node_payload, node_registers);
		if (status != RADIO_SUCCESS) goto errors;
		// Clear boot flag.
		node_status = NODE_write_register((node_payload->node), COMMON_REGISTER_ADDRESS_CONTROL_0, COMMON_REGISTER_CONTROL_0_MASK_BFC, COMMON_REGISTER_CONTROL_0_MASK_BFC, &access_status);
		NODE_exit_error(RADIO_ERROR_BASE_NODE);
	}
	else {
		if (SWREG_read_field(reg_status_0, COMMON_REGISTER_STATUS_0_MASK_ESF) != 0) {
			// Compute error stack payload.
			status = _RADIO_COMMON_build_ul_node_payload_error_stack(node_payload, node_registers);
			if (status != RADIO_SUCCESS) goto errors;
		}
	}
errors:
	return status;
}

/*******************************************************************/
RADIO_status_t RADIO_COMMON_build_ul_node_payload_action_log(RADIO_ul_node_payload_t* node_payload, RADIO_node_action_t* node_action) {
	// Local variables.
	RADIO_status_t status = RADIO_SUCCESS;
	RADIO_COMMON_ul_payload_action_log_t ul_payload_action_log;
	uint8_t idx = 0;
	// Check parameters.
	if ((node_payload == NULL) || (node_action == NULL)) {
		status = RADIO_ERROR_NULL_PARAMETER;
		goto errors;
	}
	if (((node_payload->node) == NULL) || ((node_payload->payload) == NULL)) {
		status = RADIO_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Build frame.
	ul_payload_action_log.marker = 0b1111;
	ul_payload_action_log.downlink_hash = ((node_action -> downlink_hash) & 0x00000FFF);
	ul_payload_action_log.reg_addr = (node_action -> reg_addr);
	ul_payload_action_log.reg_value = (node_action -> reg_value);
	ul_payload_action_log.node_access_status = ((node_action -> access_status).all);
	// Copy payload.
	for (idx=0 ; idx<RADIO_COMMON_UL_PAYLOAD_ACTION_LOG_SIZE ; idx++) {
		(node_payload->payload)[idx] = ul_payload_action_log.frame[idx];
	}
	node_payload->payload_size = RADIO_COMMON_UL_PAYLOAD_ACTION_LOG_SIZE;
errors:
	return status;
}
