/*
 * radio_dmm.c
 *
 *  Created on: 04 mar. 2023
 *      Author: Ludo
 */

#include "radio_dmm.h"

#include "dmm_registers.h"
#include "node.h"
#include "radio.h"
#include "radio_common.h"
#include "swreg.h"
#include "una.h"

/*** RADIO DMM local macros ***/

#define RADIO_DMM_UL_PAYLOAD_MONITORING_SIZE    7

/*** RADIO DMM local structures ***/

/*******************************************************************/
typedef enum {
    RADIO_DMM_UL_PAYLOAD_TYPE_MONITORING = 0,
    RADIO_DMM_UL_PAYLOAD_TYPE_LAST
} RADIO_DMM_ul_payload_type_t;

/*******************************************************************/
typedef union {
    uint8_t frame[RADIO_DMM_UL_PAYLOAD_MONITORING_SIZE];
    struct {
        unsigned vrs :16;
        unsigned vhmi :16;
        unsigned vusb :16;
        unsigned nodes_count :8;
    } __attribute__((scalar_storage_order("big-endian")))__attribute__((packed));
} RADIO_DMM_ul_payload_monitoring_t;

/*** RADIO DMM local global variables ***/

static const uint8_t RADIO_DMM_REGISTERS_MONITORING[] = {
    DMM_REGISTER_ADDRESS_STATUS_1,
    DMM_REGISTER_ADDRESS_ANALOG_DATA_1,
    DMM_REGISTER_ADDRESS_ANALOG_DATA_2
};

static const RADIO_DMM_ul_payload_type_t RADIO_DMM_UL_PAYLOAD_PATTERN[] = {
    RADIO_DMM_UL_PAYLOAD_TYPE_MONITORING
};

/*** RADIO DMM functions ***/

/*******************************************************************/
RADIO_status_t RADIO_DMM_build_ul_node_payload(RADIO_ul_node_payload_t* node_payload) {
    // Local variables.
    RADIO_status_t status = RADIO_SUCCESS;
    NODE_status_t node_status = NODE_SUCCESS;
    UNA_access_status_t access_status;
    uint32_t dmm_registers[DMM_REGISTER_ADDRESS_LAST];
    RADIO_DMM_ul_payload_monitoring_t ul_payload_monitoring;
    uint8_t idx = 0;
    // Check parameters.
    if (node_payload == NULL) {
        status = RADIO_ERROR_NULL_PARAMETER;
        goto errors;
    }
    if (((node_payload->node) == NULL) || ((node_payload->payload) == NULL)) {
        status = RADIO_ERROR_NULL_PARAMETER;
        goto errors;
    }
    // Reset registers.
    for (idx = 0; idx < DMM_REGISTER_ADDRESS_LAST; idx++) {
        dmm_registers[idx] = DMM_REGISTER_ERROR_VALUE[idx];
    }
    // Reset payload size.
    node_payload->payload_size = 0;
    // Check event driven payloads.
    status = RADIO_COMMON_check_event_driven_payloads(node_payload, (uint32_t*) dmm_registers);
    if (status != RADIO_SUCCESS) goto errors;
    // Directly exits if a common payload was computed.
    if (node_payload->payload_size > 0) goto errors;
    // Else use specific pattern of the node.
    switch (RADIO_DMM_UL_PAYLOAD_PATTERN[node_payload->payload_type_counter]) {
    case RADIO_DMM_UL_PAYLOAD_TYPE_MONITORING:
        // Perform measurements.
        node_status = NODE_perform_measurements((node_payload->node), &access_status);
        NODE_exit_error(RADIO_ERROR_BASE_NODE);
        // Check write status.
        if (access_status.flags == 0) {
            // Read related registers.
            node_status = NODE_read_registers((node_payload->node), (uint8_t*) RADIO_DMM_REGISTERS_MONITORING, sizeof(RADIO_DMM_REGISTERS_MONITORING), (uint32_t*) dmm_registers, &access_status);
            NODE_exit_error(RADIO_ERROR_BASE_NODE);
        }
        // Build monitoring payload.
        ul_payload_monitoring.vrs = SWREG_read_field(dmm_registers[DMM_REGISTER_ADDRESS_ANALOG_DATA_1], DMM_REGISTER_ANALOG_DATA_1_MASK_VRS);
        ul_payload_monitoring.vhmi = SWREG_read_field(dmm_registers[DMM_REGISTER_ADDRESS_ANALOG_DATA_1], DMM_REGISTER_ANALOG_DATA_1_MASK_VHMI);
        ul_payload_monitoring.vusb = SWREG_read_field(dmm_registers[DMM_REGISTER_ADDRESS_ANALOG_DATA_2], DMM_REGISTER_ANALOG_DATA_2_MASK_VUSB);
        ul_payload_monitoring.nodes_count = SWREG_read_field(dmm_registers[DMM_REGISTER_ADDRESS_STATUS_1], DMM_REGISTER_STATUS_1_MASK_NODES_COUNT);
        // Copy payload.
        for (idx = 0; idx < RADIO_DMM_UL_PAYLOAD_MONITORING_SIZE; idx++) {
            (node_payload->payload)[idx] = ul_payload_monitoring.frame[idx];
        }
        node_payload->payload_size = RADIO_DMM_UL_PAYLOAD_MONITORING_SIZE;
        break;
    default:
        status = RADIO_ERROR_UL_NODE_PAYLOAD_TYPE;
        goto errors;
    }
    // Increment payload type counter.
    node_payload->payload_type_counter = (((node_payload->payload_type_counter) + 1) % sizeof(RADIO_DMM_UL_PAYLOAD_PATTERN));
errors:
    return status;
}
