/*
 * radio_lvrm.c
 *
 *  Created on: 22 jan. 2023
 *      Author: Ludo
 */

#include "radio_lvrm.h"

#include "common_registers.h"
#include "lvrm_registers.h"
#include "radio.h"
#include "radio_common.h"
#include "swreg.h"
#include "una.h"

/*** LVRM local macros ***/

#define RADIO_LVRM_UL_PAYLOAD_MONITORING_SIZE   3
#define RADIO_LVRM_UL_PAYLOAD_ELECTRICAL_SIZE   7

/*** LVRM local structures ***/

/*******************************************************************/
typedef enum {
    RADIO_LVRM_UL_PAYLOAD_TYPE_MONITORING = 0,
    RADIO_LVRM_UL_PAYLOAD_TYPE_ELECTRICAL,
    RADIO_LVRM_UL_PAYLOAD_TYPE_LAST
} RADIO_LVRM_ul_payload_type_t;

/*******************************************************************/
typedef union {
    uint8_t frame[RADIO_LVRM_UL_PAYLOAD_MONITORING_SIZE];
    struct {
        unsigned vmcu :16;
        unsigned tmcu :8;
    } __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} RADIO_LVRM_ul_payload_monitoring_t;

/*******************************************************************/
typedef union {
    uint8_t frame[RADIO_LVRM_UL_PAYLOAD_ELECTRICAL_SIZE];
    struct {
        unsigned vin :16;
        unsigned vout :16;
        unsigned iout :16;
        unsigned unused :6;
        unsigned rlstst :2;
    } __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} RADIO_LVRM_ul_payload_electrical_t;

/*** LVRM local global variables ***/

static const uint8_t RADIO_LVRM_REGISTERS_MONITORING[] = {
    COMMON_REGISTER_ADDRESS_ANALOG_DATA_0
};

static const uint8_t RADIO_LVRM_REGISTERS_ELECTRICAL[] = {
    LVRM_REGISTER_ADDRESS_STATUS_1,
    LVRM_REGISTER_ADDRESS_ANALOG_DATA_1,
    LVRM_REGISTER_ADDRESS_ANALOG_DATA_2
};

static const RADIO_LVRM_ul_payload_type_t RADIO_LVRM_UL_PAYLOAD_PATTERN[] = {
    RADIO_LVRM_UL_PAYLOAD_TYPE_ELECTRICAL,
    RADIO_LVRM_UL_PAYLOAD_TYPE_ELECTRICAL,
    RADIO_LVRM_UL_PAYLOAD_TYPE_ELECTRICAL,
    RADIO_LVRM_UL_PAYLOAD_TYPE_ELECTRICAL,
    RADIO_LVRM_UL_PAYLOAD_TYPE_ELECTRICAL,
    RADIO_LVRM_UL_PAYLOAD_TYPE_ELECTRICAL,
    RADIO_LVRM_UL_PAYLOAD_TYPE_ELECTRICAL,
    RADIO_LVRM_UL_PAYLOAD_TYPE_ELECTRICAL,
    RADIO_LVRM_UL_PAYLOAD_TYPE_ELECTRICAL,
    RADIO_LVRM_UL_PAYLOAD_TYPE_ELECTRICAL,
    RADIO_LVRM_UL_PAYLOAD_TYPE_MONITORING
};

/*** LVRM functions ***/

/*******************************************************************/
RADIO_status_t RADIO_LVRM_build_ul_node_payload(RADIO_node_t* radio_node, RADIO_ul_payload_t* node_payload) {
    // Local variables.
    RADIO_status_t status = RADIO_SUCCESS;
    NODE_status_t node_status = NODE_SUCCESS;
    UNA_access_status_t access_status;
    uint32_t lvrm_registers[LVRM_REGISTER_ADDRESS_LAST];
    RADIO_LVRM_ul_payload_monitoring_t ul_payload_monitoring;
    RADIO_LVRM_ul_payload_electrical_t ul_payload_electrical;
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
    for (idx = 0; idx < LVRM_REGISTER_ADDRESS_LAST; idx++) {
        lvrm_registers[idx] = LVRM_REGISTER_ERROR_VALUE[idx];
    }
    // Reset payload size.
    node_payload->payload_size = 0;
    // Check event driven payloads.
    status = RADIO_COMMON_check_event_driven_payloads(radio_node, node_payload, (uint32_t*) lvrm_registers);
    if (status != RADIO_SUCCESS) goto errors;
    // Directly exits if a common payload was computed.
    if ((node_payload->payload_size) > 0) goto errors;
    // Else use specific pattern of the node.
    switch (RADIO_LVRM_UL_PAYLOAD_PATTERN[radio_node->payload_type_counter]) {
    case RADIO_LVRM_UL_PAYLOAD_TYPE_MONITORING:
        // Perform measurements.
        node_status = NODE_perform_measurements((radio_node->node), &access_status);
        NODE_exit_error(RADIO_ERROR_BASE_NODE);
        // Check write status.
        if (access_status.flags == 0) {
            // Read related registers.
            node_status = NODE_read_registers((radio_node->node), (uint8_t*) RADIO_LVRM_REGISTERS_MONITORING, sizeof(RADIO_LVRM_REGISTERS_MONITORING), (uint32_t*) lvrm_registers, &access_status);
            NODE_exit_error(RADIO_ERROR_BASE_NODE);
        }
        // Build monitoring payload.
        ul_payload_monitoring.vmcu = SWREG_read_field(lvrm_registers[COMMON_REGISTER_ADDRESS_ANALOG_DATA_0], COMMON_REGISTER_ANALOG_DATA_0_MASK_VMCU);
        ul_payload_monitoring.tmcu = SWREG_read_field(lvrm_registers[COMMON_REGISTER_ADDRESS_ANALOG_DATA_0], COMMON_REGISTER_ANALOG_DATA_0_MASK_TMCU);
        // Copy payload.
        for (idx = 0; idx < RADIO_LVRM_UL_PAYLOAD_MONITORING_SIZE; idx++) {
            (node_payload->payload)[idx] = ul_payload_monitoring.frame[idx];
        }
        node_payload->payload_size = RADIO_LVRM_UL_PAYLOAD_MONITORING_SIZE;
        break;
    case RADIO_LVRM_UL_PAYLOAD_TYPE_ELECTRICAL:
        // Perform measurements.
        node_status = NODE_perform_measurements((radio_node->node), &access_status);
        NODE_exit_error(RADIO_ERROR_BASE_NODE);
        // Check write status.
        if (access_status.flags == 0) {
            // Read related registers.
            node_status = NODE_read_registers((radio_node->node), (uint8_t*) RADIO_LVRM_REGISTERS_ELECTRICAL, sizeof(RADIO_LVRM_REGISTERS_ELECTRICAL), (uint32_t*) lvrm_registers, &access_status);
            NODE_exit_error(RADIO_ERROR_BASE_NODE);
        }
        // Build data payload.
        ul_payload_electrical.vin = SWREG_read_field(lvrm_registers[LVRM_REGISTER_ADDRESS_ANALOG_DATA_1], LVRM_REGISTER_ANALOG_DATA_1_MASK_VCOM);
        ul_payload_electrical.vout = SWREG_read_field(lvrm_registers[LVRM_REGISTER_ADDRESS_ANALOG_DATA_1], LVRM_REGISTER_ANALOG_DATA_1_MASK_VOUT);
        ul_payload_electrical.iout = SWREG_read_field(lvrm_registers[LVRM_REGISTER_ADDRESS_ANALOG_DATA_2], LVRM_REGISTER_ANALOG_DATA_2_MASK_IOUT);
        ul_payload_electrical.unused = 0;
        ul_payload_electrical.rlstst = SWREG_read_field(lvrm_registers[LVRM_REGISTER_ADDRESS_STATUS_1], LVRM_REGISTER_STATUS_1_MASK_RLSTST);
        // Copy payload.
        for (idx = 0; idx < RADIO_LVRM_UL_PAYLOAD_ELECTRICAL_SIZE; idx++) {
            (node_payload->payload)[idx] = ul_payload_electrical.frame[idx];
        }
        node_payload->payload_size = RADIO_LVRM_UL_PAYLOAD_ELECTRICAL_SIZE;
        break;
    default:
        status = RADIO_ERROR_UL_NODE_PAYLOAD_TYPE;
        goto errors;
    }
    // Increment payload type counter.
    radio_node->payload_type_counter = (((radio_node->payload_type_counter) + 1) % sizeof(RADIO_LVRM_UL_PAYLOAD_PATTERN));
errors:
    return status;
}
