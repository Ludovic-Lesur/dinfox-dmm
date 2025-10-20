/*
 * radio_gpsm.c
 *
 *  Created on: 16 apr. 2023
 *      Author: Ludo
 */

#include "radio_gpsm.h"

#include "common_registers.h"
#include "gpsm_registers.h"
#include "radio.h"
#include "radio_common.h"
#include "swreg.h"
#include "una.h"

/*** GPSM local macros ***/

#define RADIO_GPSM_UL_PAYLOAD_MONITORING_SIZE   7

/*** GPSM local structures ***/

/*******************************************************************/
typedef enum {
    RADIO_GPSM_UL_PAYLOAD_TYPE_MONITORING = 0,
    RADIO_GPSM_UL_PAYLOAD_TYPE_LAST
} RADIO_GPSM_ul_payload_type_t;

/*******************************************************************/
typedef union {
    uint8_t frame[RADIO_GPSM_UL_PAYLOAD_MONITORING_SIZE];
    struct {
        unsigned vmcu :16;
        unsigned tmcu :8;
        unsigned vgps :16;
        unsigned vant :16;
    } __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} RADIO_GPSM_ul_payload_monitoring_t;

/*** GPSM local global variables ***/

static const uint8_t RADIO_GPSM_REGISTERS_MONITORING[] = {
    COMMON_REGISTER_ADDRESS_ANALOG_DATA_0,
    GPSM_REGISTER_ADDRESS_ANALOG_DATA_1
};

static const RADIO_GPSM_ul_payload_type_t RADIO_GPSM_UL_PAYLOAD_PATTERN[] = {
    RADIO_GPSM_UL_PAYLOAD_TYPE_MONITORING
};

/*** GPSM functions ***/

/*******************************************************************/
RADIO_status_t RADIO_GPSM_build_ul_node_payload(RADIO_node_t* radio_node, RADIO_ul_payload_t* node_payload) {
    // Local variables.
    RADIO_status_t status = RADIO_SUCCESS;
    NODE_status_t node_status = NODE_SUCCESS;
    UNA_access_status_t access_status;
    uint32_t gpsm_registers[GPSM_REGISTER_ADDRESS_LAST];
    RADIO_GPSM_ul_payload_monitoring_t ul_payload_monitoring;
    uint8_t idx = 0;
    // Check parameters.
    if ((radio_node == NULL) || (node_payload == NULL)) {
        status = RADIO_ERROR_NULL_PARAMETER;
        goto errors;
    }
    if (((radio_node->node) == NULL) || ((node_payload->payload) == NULL)) {
        status = RADIO_ERROR_NULL_PARAMETER;
        goto errors;
    }
    // Reset registers.
    for (idx = 0; idx < GPSM_REGISTER_ADDRESS_LAST; idx++) {
        gpsm_registers[idx] = GPSM_REGISTER_ERROR_VALUE[idx];
    }
    // Reset payload size.
    node_payload->payload_size = 0;
    // Check event driven payloads.
    status = RADIO_COMMON_check_event_driven_payloads(radio_node, node_payload, (uint32_t*) gpsm_registers);
    if (status != RADIO_SUCCESS) goto errors;
    // Directly exits if a common payload was computed.
    if ((node_payload->payload_size) > 0) goto errors;
    // Else use specific pattern of the node.
    switch (RADIO_GPSM_UL_PAYLOAD_PATTERN[radio_node->payload_type_counter]) {
    case RADIO_GPSM_UL_PAYLOAD_TYPE_MONITORING:
        // Perform measurements.
        node_status = NODE_perform_measurements((radio_node->node), &access_status);
        NODE_exit_error(RADIO_ERROR_BASE_NODE);
        // Check write status.
        if (access_status.flags == 0) {
            // Read related registers.
            node_status = NODE_read_registers((radio_node->node), (uint8_t*) RADIO_GPSM_REGISTERS_MONITORING, sizeof(RADIO_GPSM_REGISTERS_MONITORING), (uint32_t*) gpsm_registers, &access_status);
            NODE_exit_error(RADIO_ERROR_BASE_NODE);
        }
        // Build monitoring payload.
        ul_payload_monitoring.vmcu = SWREG_read_field(gpsm_registers[COMMON_REGISTER_ADDRESS_ANALOG_DATA_0], COMMON_REGISTER_ANALOG_DATA_0_MASK_VMCU);
        ul_payload_monitoring.tmcu = (SWREG_read_field(gpsm_registers[COMMON_REGISTER_ADDRESS_ANALOG_DATA_0], COMMON_REGISTER_ANALOG_DATA_0_MASK_TMCU) / 10);
        ul_payload_monitoring.vgps = SWREG_read_field(gpsm_registers[GPSM_REGISTER_ADDRESS_ANALOG_DATA_1], GPSM_REGISTER_ANALOG_DATA_1_MASK_VGPS);
        ul_payload_monitoring.vant = SWREG_read_field(gpsm_registers[GPSM_REGISTER_ADDRESS_ANALOG_DATA_1], GPSM_REGISTER_ANALOG_DATA_1_MASK_VANT);
        // Copy payload.
        for (idx = 0; idx < RADIO_GPSM_UL_PAYLOAD_MONITORING_SIZE; idx++) {
            (node_payload->payload)[idx] = ul_payload_monitoring.frame[idx];
        }
        node_payload->payload_size = RADIO_GPSM_UL_PAYLOAD_MONITORING_SIZE;
        break;
    default:
        status = RADIO_ERROR_UL_NODE_PAYLOAD_TYPE;
        goto errors;
    }
    // Increment payload type counter.
    radio_node->payload_type_counter = (((radio_node->payload_type_counter) + 1) % sizeof(RADIO_GPSM_UL_PAYLOAD_PATTERN));
errors:
    return status;
}
