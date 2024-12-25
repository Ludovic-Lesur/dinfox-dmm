/*
 * node.c
 *
 *  Created on: 23 jan. 2023
 *      Author: Ludo
 */

#include "node.h"

#include "bpsm_registers.h"
#include "ddrm_registers.h"
#include "dmm_registers.h"
#include "error.h"
#include "error_base.h"
#include "gpio.h"
#include "gpio_mapping.h"
#include "gpsm_registers.h"
#include "lpuart.h"
#include "lvrm_registers.h"
#include "mode.h"
#include "mpmcm_registers.h"
#include "r4s8cr_registers.h"
#include "rrm_registers.h"
#include "rtc.h"
#include "sm_registers.h"
#include "swreg.h"
#include "uhfm_registers.h"
#include "una.h"
#include "una_at.h"
#include "una_dmm.h"
#include "una_r4s8cr.h"

/*** NODE local macros ***/

#define NODE_LIST_DMM_NODE_INDEX            0
#define NODE_UNA_AT_BAUD_RATE               1200

#define NODE_SCAN_PERIOD_DEFAULT_SECONDS    86400

/*** NODE local structures ***/

/*******************************************************************/
typedef enum {
    NODE_PROTOCOL_UNA_DMM = 0,
    NODE_PROTOCOL_UNA_AT,
    NODE_PROTOCOL_UNA_R4S8CR,
    NODE_PROTOCOL_LAST
} NODE_protocol_t;

/*******************************************************************/
typedef struct {
    NODE_protocol_t protocol;
	uint8_t register_address_last;
	uint32_t* register_write_timeout_ms;
} NODE_descriptor_t;

/*******************************************************************/
typedef struct {
	uint32_t scan_next_time_seconds;
} NODE_context_t;

/*** NODE global variables ***/

UNA_node_list_t NODE_LIST;

/*** NODE local global variables ***/

static const NODE_descriptor_t NODES[UNA_BOARD_ID_LAST] = {
	{ NODE_PROTOCOL_UNA_AT, LVRM_REGISTER_ADDRESS_LAST, (uint32_t*) LVRM_REGISTER_ACCESS_TIMEOUT_MS },
	{ NODE_PROTOCOL_UNA_AT, BPSM_REGISTER_ADDRESS_LAST, (uint32_t*) BPSM_REGISTER_ACCESS_TIMEOUT_MS },
	{ NODE_PROTOCOL_UNA_AT, DDRM_REGISTER_ADDRESS_LAST, (uint32_t*) DDRM_REGISTER_ACCESS_TIMEOUT_MS },
	{ NODE_PROTOCOL_UNA_AT, UHFM_REGISTER_ADDRESS_LAST, (uint32_t*) UHFM_REGISTER_ACCESS_TIMEOUT_MS },
	{ NODE_PROTOCOL_UNA_AT, GPSM_REGISTER_ADDRESS_LAST, (uint32_t*) GPSM_REGISTER_ACCESS_TIMEOUT_MS },
	{ NODE_PROTOCOL_UNA_AT, SM_REGISTER_ADDRESS_LAST, (uint32_t*) SM_REGISTER_ACCESS_TIMEOUT_MS },
	{ NODE_PROTOCOL_UNA_AT, 0, NULL },
	{ NODE_PROTOCOL_UNA_AT, RRM_REGISTER_ADDRESS_LAST, (uint32_t*) RRM_REGISTER_ACCESS_TIMEOUT_MS },
	{ NODE_PROTOCOL_UNA_DMM, DMM_REGISTER_ADDRESS_LAST, (uint32_t*) DMM_REGISTER_ACCESS_TIMEOUT_MS },
	{ NODE_PROTOCOL_UNA_AT, MPMCM_REGISTER_ADDRESS_LAST, (uint32_t*) MPMCM_REGISTER_ACCESS_TIMEOUT_MS },
	{ NODE_PROTOCOL_UNA_R4S8CR, R4S8CR_REGISTER_ADDRESS_LAST, (uint32_t*) R4S8CR_REGISTER_ACCESS_TIMEOUT_MS }
};

static NODE_context_t node_ctx;

/*** NODE local functions ***/

/*******************************************************************/
#define _NODE_check_node_and_board_id(void) { \
	if (node == NULL) { \
		status = NODE_ERROR_NULL_PARAMETER; \
		goto errors; \
	} \
	if ((node->board_id) >= UNA_BOARD_ID_LAST) { \
		status = NODE_ERROR_NOT_SUPPORTED; \
		goto errors; \
	} \
	if (NODES[node->board_id].register_address_last == 0) { \
		status = NODE_ERROR_NOT_SUPPORTED; \
		goto errors; \
	} \
}

/*** NODE functions ***/

/*******************************************************************/
NODE_status_t NODE_init(void) {
	// Local variables.
    NODE_status_t status = NODE_SUCCESS;
    UNA_DMM_status_t una_dmm_status = UNA_DMM_SUCCESS;
    // Init context.
    node_ctx.scan_next_time_seconds = 0;
	// Reset node list.
	UNA_reset_node_list(&NODE_LIST);
	// Init self registers.
	una_dmm_status = UNA_DMM_init();
	UNA_DMM_exit_error(NODE_ERROR_BASE_UNA_DMM);
errors:
	return status;
}

/*******************************************************************/
NODE_status_t NODE_de_init(void) {
    // Local variables.
    NODE_status_t status = NODE_SUCCESS;
    return status;
}

/*******************************************************************/
NODE_status_t NODE_write_register(UNA_node_t* node, uint8_t reg_addr, uint32_t reg_value, uint32_t reg_mask, UNA_access_status_t* write_status) {
    // Local variables.
    NODE_status_t status = NODE_SUCCESS;
    UNA_DMM_status_t una_dmm_status = UNA_DMM_SUCCESS;
    UNA_AT_status_t una_at_status = UNA_AT_SUCCESS;
    UNA_R4S8CR_status_t una_r4s8cr_status = UNA_R4S8CR_SUCCESS;
    UNA_access_parameters_t write_params;
    uint8_t una_at_init = 0;
    uint8_t una_r4s8cr_init = 0;
    // Check node and board ID.
    _NODE_check_node_and_board_id();
    // Common write parameters.
    write_params.node_addr = (node -> address);
    write_params.reg_addr = reg_addr;
    write_params.reply_params.timeout_ms = NODES[node -> board_id].register_write_timeout_ms[reg_addr];
    write_params.reply_params.type = UNA_REPLY_TYPE_OK;
    // Check protocol.
    switch (NODES[node->board_id].protocol) {
    case NODE_PROTOCOL_UNA_DMM:
        // Write DMM node register.
        una_dmm_status = UNA_DMM_write_register(&write_params, reg_value, reg_mask, write_status);
        UNA_DMM_exit_error(NODE_ERROR_BASE_UNA_DMM);
        break;
    case NODE_PROTOCOL_UNA_AT:
        // Write UNA AT node register.
        una_at_init = 1;
        una_at_status = UNA_AT_init(NODE_UNA_AT_BAUD_RATE);
        UNA_AT_exit_error(NODE_ERROR_BASE_UNA_AT);
        una_at_status = UNA_AT_write_register(&write_params, reg_value, reg_mask, write_status);
        UNA_AT_exit_error(NODE_ERROR_BASE_UNA_AT);
        una_at_status = UNA_AT_de_init();
        UNA_AT_exit_error(NODE_ERROR_BASE_UNA_AT);
        una_at_init = 0;
        break;
    case NODE_PROTOCOL_UNA_R4S8CR:
        // Write UNA R4S8CR node register.
        una_r4s8cr_init = 1;
        una_r4s8cr_status = UNA_R4S8CR_init();
        UNA_R4S8CR_exit_error(NODE_ERROR_BASE_UNA_R4S8CR);
        una_r4s8cr_status = UNA_R4S8CR_write_register(&write_params, reg_value, reg_mask, write_status);
        UNA_R4S8CR_exit_error(NODE_ERROR_BASE_UNA_R4S8CR);
        una_r4s8cr_status = UNA_R4S8CR_de_init();
        UNA_R4S8CR_exit_error(NODE_ERROR_BASE_UNA_R4S8CR);
        una_r4s8cr_init = 0;
        break;
    default:
        status = NODE_ERROR_PROTOCOL;
        goto errors;
    }
errors:
    // Force release in case of error.
    if (una_at_init != 0) {
        UNA_AT_de_init();
    }
    if (una_r4s8cr_init != 0) {
        UNA_R4S8CR_de_init();
    }
    return status;
}

/*******************************************************************/
NODE_status_t NODE_perform_measurements(UNA_node_t* node, UNA_access_status_t* write_status) {
    // Local variables.
    NODE_status_t status = NODE_SUCCESS;
    // Check node and board ID.
    _NODE_check_node_and_board_id();
    // Check protocol.
    switch (NODES[node->board_id].protocol) {
    case NODE_PROTOCOL_UNA_DMM:
    case NODE_PROTOCOL_UNA_AT:
        status = NODE_write_register(node, COMMON_REGISTER_ADDRESS_CONTROL_0, COMMON_REGISTER_CONTROL_0_MASK_MTRG, COMMON_REGISTER_CONTROL_0_MASK_MTRG, write_status);
        if (status != NODE_SUCCESS) goto errors;
        break;
    case NODE_PROTOCOL_UNA_R4S8CR:
        // Nothing to do.
        break;
    default:
        status = NODE_ERROR_PROTOCOL;
        goto errors;
    }
errors:
    return status;
}

/*******************************************************************/
NODE_status_t NODE_read_register(UNA_node_t* node, uint8_t reg_addr, uint32_t* reg_value, UNA_access_status_t* read_status) {
    // Local variables.
    NODE_status_t status = NODE_SUCCESS;
    UNA_DMM_status_t una_dmm_status = UNA_DMM_SUCCESS;
    UNA_AT_status_t una_at_status = UNA_AT_SUCCESS;
    UNA_R4S8CR_status_t una_r4s8cr_status = UNA_R4S8CR_SUCCESS;
    UNA_access_parameters_t read_params;
    uint8_t una_at_init = 0;
    uint8_t una_r4s8cr_init = 0;
    // Check node and board ID.
    _NODE_check_node_and_board_id();
    // Write parameters.
    read_params.node_addr = (node -> address);
    read_params.reg_addr = reg_addr;
    read_params.reply_params.timeout_ms = NODES[node -> board_id].register_write_timeout_ms[reg_addr];
    read_params.reply_params.type = UNA_REPLY_TYPE_VALUE;
    // Check protocol.
    switch (NODES[node->board_id].protocol) {
    case NODE_PROTOCOL_UNA_DMM:
        // Write DMM node register.
        una_dmm_status = UNA_DMM_read_register(&read_params, reg_value, read_status);
        UNA_DMM_exit_error(NODE_ERROR_BASE_UNA_DMM);
        break;
    case NODE_PROTOCOL_UNA_AT:
        // Write UNA AT node register.
        una_at_init = 1;
        una_at_status = UNA_AT_init(NODE_UNA_AT_BAUD_RATE);
        UNA_AT_exit_error(NODE_ERROR_BASE_UNA_AT);
        una_at_status = UNA_AT_read_register(&read_params, reg_value, read_status);
        UNA_AT_exit_error(NODE_ERROR_BASE_UNA_AT);
        una_at_status = UNA_AT_de_init();
        UNA_AT_exit_error(NODE_ERROR_BASE_UNA_AT);
        una_at_init = 0;
        break;
    case NODE_PROTOCOL_UNA_R4S8CR:
        // Write UNA R4S8CR node register.
        una_r4s8cr_init = 1;
        una_r4s8cr_status = UNA_R4S8CR_init();
        UNA_R4S8CR_exit_error(NODE_ERROR_BASE_UNA_R4S8CR);
        una_r4s8cr_status = UNA_R4S8CR_read_register(&read_params, reg_value, read_status);
        UNA_R4S8CR_exit_error(NODE_ERROR_BASE_UNA_R4S8CR);
        una_r4s8cr_status = UNA_R4S8CR_de_init();
        UNA_R4S8CR_exit_error(NODE_ERROR_BASE_UNA_R4S8CR);
        una_r4s8cr_init = 0;
        break;
    default:
        status = NODE_ERROR_PROTOCOL;
        goto errors;
    }
errors:
    // Force release in case of error.
    if (una_at_init != 0) {
        UNA_AT_de_init();
    }
    if (una_r4s8cr_init != 0) {
        UNA_R4S8CR_de_init();
    }
    // Store eventual access status error.
    if ((read_status->flags) != 0){
        ERROR_stack_add(ERROR_BASE_NODE + NODE_ERROR_BASE_ACCESS_STATUS_CODE + (read_status -> all));
        ERROR_stack_add(ERROR_BASE_NODE + NODE_ERROR_BASE_ACCESS_STATUS_ADDRESS + (node->address));
    }
    return status;
}

/*******************************************************************/
NODE_status_t NODE_read_registers(UNA_node_t* node, uint8_t* reg_addr_list, uint8_t reg_addr_list_size, uint32_t* node_registers, UNA_access_status_t* read_status) {
    // Local variables.
    NODE_status_t status = NODE_SUCCESS;
    uint8_t reg_addr = 0;
    uint8_t idx = 0;
    // Check node and board ID.
    _NODE_check_node_and_board_id();
    // Check parameters.
    if ((reg_addr_list == NULL) || (node_registers == NULL) || (read_status == NULL)) {
        status = NODE_ERROR_NULL_PARAMETER;
        goto errors;
    }
    if (reg_addr_list_size == 0) {
        status = NODE_ERROR_REGISTER_ADDRESS_LIST_SIZE;
        goto errors;
    }
    // Registers loop.
    for (idx = 0; idx < reg_addr_list_size; idx++) {
        // Update register address.
        reg_addr = reg_addr_list[idx];
        // Read register.
        status = NODE_read_register(node, reg_addr, &(node_registers[reg_addr]), read_status);
        if (status != NODE_SUCCESS) goto errors;
    }
errors:
    return status;
}

/*******************************************************************/
NODE_status_t NODE_scan(void) {
    // Local variables.
    NODE_status_t status = NODE_SUCCESS;
    POWER_status_t power_status = POWER_SUCCESS;
    UNA_AT_status_t una_at_status = UNA_AT_SUCCESS;
    UNA_R4S8CR_status_t una_r4s8cr_status = UNA_R4S8CR_SUCCESS;
    UNA_DMM_status_t una_dmm_status = UNA_DMM_SUCCESS;
    UNA_access_parameters_t read_params;
    UNA_access_status_t read_status;
    uint32_t reg_value = 0;
    uint32_t scan_period_seconds = NODE_SCAN_PERIOD_DEFAULT_SECONDS;
    uint8_t nodes_count = 0;
    uint8_t una_at_init = 0;
    uint8_t una_r4s8cr_init = 0;
    // Reset list.
    UNA_reset_node_list(&NODE_LIST);
    // Add master board to the list.
    NODE_LIST.list[NODE_LIST_DMM_NODE_INDEX].board_id = UNA_BOARD_ID_DMM;
    NODE_LIST.list[NODE_LIST_DMM_NODE_INDEX].address = UNA_NODE_ADDRESS_MASTER;
    NODE_LIST.count++;
    // Turn bus interface on.
    power_status = POWER_enable(POWER_REQUESTER_ID_NODE, POWER_DOMAIN_RS485, LPTIM_DELAY_MODE_STOP);
    POWER_exit_error(NODE_ERROR_BASE_POWER);
    // Scan LBUS nodes.
    una_at_init = 1;
    una_at_status = UNA_AT_init(NODE_UNA_AT_BAUD_RATE);
    UNA_AT_exit_error(NODE_ERROR_BASE_UNA_AT);
    una_at_status = UNA_AT_scan(&(NODE_LIST.list[NODE_LIST.count]), (NODE_LIST_SIZE - NODE_LIST.count), &nodes_count);
    UNA_AT_exit_error(NODE_ERROR_BASE_UNA_AT);
    una_at_status = UNA_AT_de_init();
    UNA_AT_exit_error(NODE_ERROR_BASE_UNA_AT);
    una_at_init = 0;
    // Update count.
    NODE_LIST.count += nodes_count;
    // Scan R4S8CR nodes.
    una_r4s8cr_init = 1;
    una_r4s8cr_status = UNA_R4S8CR_init();
    UNA_R4S8CR_exit_error(NODE_ERROR_BASE_UNA_R4S8CR);
    una_r4s8cr_status = UNA_R4S8CR_scan(&(NODE_LIST.list[NODE_LIST.count]), (NODE_LIST_SIZE - NODE_LIST.count), &nodes_count);
    UNA_R4S8CR_exit_error(NODE_ERROR_BASE_UNA_R4S8CR);
    una_r4s8cr_status = UNA_R4S8CR_de_init();
    UNA_R4S8CR_exit_error(NODE_ERROR_BASE_UNA_R4S8CR);
    una_r4s8cr_init = 0;
    // Update count.
    NODE_LIST.count += nodes_count;
errors:
    // Force release in case of error.
    if (una_at_init != 0) {
        UNA_AT_de_init();
    }
    if (una_r4s8cr_init != 0) {
        UNA_R4S8CR_de_init();
    }
    // Turn bus interface off.
    power_status = POWER_disable(POWER_REQUESTER_ID_NODE, POWER_DOMAIN_RS485);
    POWER_stack_error(ERROR_BASE_POWER);
    // Read scan period.
    read_params.node_addr = UNA_NODE_ADDRESS_MASTER;
    read_params.reg_addr = DMM_REGISTER_ADDRESS_CONFIGURATION_0;
    read_params.reply_params.type = UNA_REPLY_TYPE_OK;
    read_params.reply_params.timeout_ms = 0;
    una_dmm_status = UNA_DMM_read_register(&read_params, &reg_value, &read_status);
    UNA_DMM_stack_error(ERROR_BASE_UNA_DMM);
    // Check access status.
    if ((una_dmm_status == UNA_DMM_SUCCESS) && (read_status.all == 0)) {
        scan_period_seconds = UNA_get_seconds((uint32_t) SWREG_read_field(reg_value, DMM_REGISTER_CONFIGURATION_0_MASK_NODES_SCAN_PERIOD));
    }
    // Update next scan time.
    node_ctx.scan_next_time_seconds += scan_period_seconds;
    return status;
}

/*******************************************************************/
NODE_status_t NODE_process(void) {
    // Local variables.
    NODE_status_t status = NODE_SUCCESS;
    // Check scan period.
    if (RTC_get_uptime_seconds() >= node_ctx.scan_next_time_seconds) {
        // Perform scan.
        status = NODE_scan();
        if (status != NODE_SUCCESS) goto errors;
    }
errors:
    return status;
}
