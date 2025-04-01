/*
 * uhfm.c
 *
 *  Created on: 26 feb. 2023
 *      Author: Ludo
 */

#include "radio_uhfm.h"

#include "common_registers.h"
#include "radio.h"
#include "radio_common.h"
#include "swreg.h"
#include "uhfm_registers.h"
#include "una.h"

/*** UHFM local macros ***/

#define RADIO_UHFM_UL_PAYLOAD_MONITORING_SIZE   7

/*** UHFM local structures ***/

/*******************************************************************/
typedef enum {
    RADIO_UHFM_UL_PAYLOAD_TYPE_MONITORING = 0,
    RADIO_UHFM_UL_PAYLOAD_TYPE_LAST
} RADIO_UHFM_ul_payload_type_t;

/*******************************************************************/
typedef union {
    uint8_t frame[RADIO_UHFM_UL_PAYLOAD_MONITORING_SIZE];
    struct {
        unsigned vmcu :16;
        unsigned tmcu :8;
        unsigned vrf_tx :16;
        unsigned vrf_rx :16;
    } __attribute__((scalar_storage_order("big-endian")))__attribute__((packed));
} RADIO_UHFM_ul_payload_monitoring_t;

/*** UHFM global variables ***/

static const uint8_t RADIO_UHFM_REGISTERS_MONITORING[] = {
    COMMON_REGISTER_ADDRESS_ANALOG_DATA_0,
    UHFM_REGISTER_ADDRESS_ANALOG_DATA_1
};

static const RADIO_UHFM_ul_payload_type_t RADIO_UHFM_UL_PAYLOAD_PATTERN[] = {
    RADIO_UHFM_UL_PAYLOAD_TYPE_MONITORING
};

/*** UHFM functions ***/

/*******************************************************************/
RADIO_status_t RADIO_UHFM_build_ul_node_payload(RADIO_ul_node_payload_t* node_payload) {
    // Local variables.
    RADIO_status_t status = RADIO_SUCCESS;
    NODE_status_t node_status = NODE_SUCCESS;
    UNA_access_status_t access_status;
    uint32_t uhfm_registers[UHFM_REGISTER_ADDRESS_LAST];
    RADIO_UHFM_ul_payload_monitoring_t ul_payload_monitoring;
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
    for (idx = 0; idx < UHFM_REGISTER_ADDRESS_LAST; idx++) {
        uhfm_registers[idx] = UHFM_REGISTER_ERROR_VALUE[idx];
    }
    // Reset payload size.
    node_payload->payload_size = 0;
    // Check event driven payloads.
    status = RADIO_COMMON_check_event_driven_payloads(node_payload, (uint32_t*) uhfm_registers);
    if (status != RADIO_SUCCESS) goto errors;
    // Directly exits if a common payload was computed.
    if ((node_payload->payload_size) > 0) goto errors;
    // Else use specific pattern of the node.
    switch (RADIO_UHFM_UL_PAYLOAD_PATTERN[node_payload->payload_type_counter]) {
    case RADIO_UHFM_UL_PAYLOAD_TYPE_MONITORING:
        // Perform measurements.
        node_status = NODE_perform_measurements((node_payload->node), &access_status);
        NODE_exit_error(RADIO_ERROR_BASE_NODE);
        // Check write status.
        if (access_status.flags == 0) {
            // Read related registers.
            node_status = NODE_read_registers((node_payload->node), (uint8_t*) RADIO_UHFM_REGISTERS_MONITORING, sizeof(RADIO_UHFM_REGISTERS_MONITORING), (uint32_t*) uhfm_registers, &access_status);
            NODE_exit_error(RADIO_ERROR_BASE_NODE);
        }
        // Build monitoring payload.
        ul_payload_monitoring.vmcu = SWREG_read_field(uhfm_registers[COMMON_REGISTER_ADDRESS_ANALOG_DATA_0], COMMON_REGISTER_ANALOG_DATA_0_MASK_VMCU);
        ul_payload_monitoring.tmcu = SWREG_read_field(uhfm_registers[COMMON_REGISTER_ADDRESS_ANALOG_DATA_0], COMMON_REGISTER_ANALOG_DATA_0_MASK_TMCU);
        ul_payload_monitoring.vrf_tx = SWREG_read_field(uhfm_registers[UHFM_REGISTER_ADDRESS_ANALOG_DATA_1], UHFM_REGISTER_ANALOG_DATA_1_MASK_VRF_TX);
        ul_payload_monitoring.vrf_rx = SWREG_read_field(uhfm_registers[UHFM_REGISTER_ADDRESS_ANALOG_DATA_1], UHFM_REGISTER_ANALOG_DATA_1_MASK_VRF_RX);
        // Copy payload.
        for (idx = 0; idx < RADIO_UHFM_UL_PAYLOAD_MONITORING_SIZE; idx++) {
            (node_payload->payload)[idx] = ul_payload_monitoring.frame[idx];
        }
        node_payload->payload_size = RADIO_UHFM_UL_PAYLOAD_MONITORING_SIZE;
        break;
    default:
        status = RADIO_ERROR_UL_NODE_PAYLOAD_TYPE;
        goto errors;
    }
    // Increment payload type counter.
    node_payload->payload_type_counter = (((node_payload->payload_type_counter) + 1) % sizeof(RADIO_UHFM_UL_PAYLOAD_PATTERN));
errors:
    return status;
}

/*******************************************************************/
RADIO_status_t RADIO_UHFM_send_ul_message(UNA_node_t* uhfm_node, UHFM_ul_message_t* ul_message) {
    // Local variables.
    RADIO_status_t status = RADIO_SUCCESS;
    NODE_status_t node_status = NODE_SUCCESS;
    UNA_access_status_t access_status;
    uint32_t reg_configuration_0 = 0;
    uint32_t reg_configuration_0_mask = 0;
    uint32_t ul_payload_x = 0;
    uint8_t reg_offset = 0;
    uint8_t idx = 0;
    // Check parameters.
    if ((uhfm_node == NULL) || (ul_message == NULL)) {
        status = RADIO_ERROR_NULL_PARAMETER;
        goto errors;
    }
    // Configuration register.
    SWREG_write_field(&reg_configuration_0, &reg_configuration_0_mask, (uint32_t) (ul_message->ul_payload_size), UHFM_REGISTER_CONFIGURATION_0_MASK_UL_PAYLOAD_SIZE);
    SWREG_write_field(&reg_configuration_0, &reg_configuration_0_mask, (uint32_t) (ul_message->bidirectional_flag), UHFM_REGISTER_CONFIGURATION_0_MASK_BF);
    SWREG_write_field(&reg_configuration_0, &reg_configuration_0_mask, (uint32_t) UHFM_UL_MESSAGE_TYPE_BYTE_ARRAY, UHFM_REGISTER_CONFIGURATION_0_MASK_MSGT);
    SWREG_write_field(&reg_configuration_0, &reg_configuration_0_mask, (uint32_t) 0, UHFM_REGISTER_CONFIGURATION_0_MASK_CMSG);
    SWREG_write_field(&reg_configuration_0, &reg_configuration_0_mask, (uint32_t) 0b11, UHFM_REGISTER_CONFIGURATION_0_MASK_NFR);
    SWREG_write_field(&reg_configuration_0, &reg_configuration_0_mask, (uint32_t) 0b01, UHFM_REGISTER_CONFIGURATION_0_MASK_BR);
    // Write register.
    node_status = NODE_write_register(uhfm_node, UHFM_REGISTER_ADDRESS_CONFIGURATION_0, reg_configuration_0, reg_configuration_0_mask, &access_status);
    NODE_exit_error(RADIO_ERROR_BASE_NODE);
    // Check access status.
    if ((access_status.flags) != 0) {
        status = RADIO_ERROR_MODEM_UL_CONFIGURATION;
        goto errors;
    }
    // UL payload.
    for (idx = 0; idx < (ul_message->ul_payload_size); idx++) {
        // Build register.
        ul_payload_x |= (((ul_message->ul_payload)[idx]) << ((idx % 4) << 3));
        // Check index.
        if ((((idx + 1) % 4) == 0) || (idx == ((ul_message->ul_payload_size) - 1))) {
            // Write register.
            node_status = NODE_write_register(uhfm_node, (UHFM_REGISTER_ADDRESS_UL_PAYLOAD_0 + reg_offset), ul_payload_x, UNA_REGISTER_MASK_ALL, &access_status);
            NODE_exit_error(RADIO_ERROR_BASE_NODE);
            // Check access status.
            if ((access_status.flags) != 0) {
                status = RADIO_ERROR_MODEM_UL_PAYLOAD;
                goto errors;
            }
            // Go to next register and reset value.
            reg_offset++;
            ul_payload_x = 0;
        }
    }
    // Send message.
    node_status = NODE_write_register(uhfm_node, UHFM_REGISTER_ADDRESS_CONTROL_1, UHFM_REGISTER_CONTROL_1_MASK_STRG, UHFM_REGISTER_CONTROL_1_MASK_STRG, &access_status);
    NODE_exit_error(RADIO_ERROR_BASE_NODE);
    // Check access status.
    if ((access_status.flags) != 0) {
        status = RADIO_ERROR_MODEM_UL_TRANSMISSION;
        goto errors;
    }
errors:
    return status;
}

/*******************************************************************/
RADIO_status_t RADIO_UHFM_get_dl_payload(UNA_node_t* uhfm_node, uint8_t* dl_payload_available, uint8_t* dl_payload) {
    // Local variables.
    RADIO_status_t status = RADIO_SUCCESS;
    NODE_status_t node_status = NODE_SUCCESS;
    UNA_access_status_t access_status;
    uint32_t uhfm_registers[UHFM_REGISTER_ADDRESS_LAST];
    UHFM_ul_message_status_t message_status;
    uint8_t reg_addr = (UHFM_REGISTER_ADDRESS_DL_PAYLOAD_0 - 1);
    uint8_t idx = 0;
    // Check parameters.
    if ((uhfm_node == NULL) || (dl_payload_available == NULL) || (dl_payload == NULL)) {
        status = RADIO_ERROR_NULL_PARAMETER;
        goto errors;
    }
    // Reset output flag.
    (*dl_payload_available) = 0;
    // Read message status.
    node_status = NODE_read_register(uhfm_node, UHFM_REGISTER_ADDRESS_STATUS_1, &(uhfm_registers[UHFM_REGISTER_ADDRESS_STATUS_1]), &access_status);
    NODE_exit_error(RADIO_ERROR_BASE_NODE);
    // Check access status.
    if ((access_status.flags) != 0) {
        status = RADIO_ERROR_MODEM_DL_MESSAGE_STATUS;
        goto errors;
    }
    // Compute message status.
    message_status.all = SWREG_read_field(uhfm_registers[UHFM_REGISTER_ADDRESS_STATUS_1], UHFM_REGISTER_STATUS_1_MASK_MESSAGE_STATUS);
    // Check DL flag.
    if (message_status.field.dl_frame == 0) goto errors;
    // Byte loop.
    for (idx = 0; idx < UHFM_DL_PAYLOAD_SIZE_BYTES; idx++) {
        // Check index.
        if ((idx % 4) == 0) {
            // Go to next register.
            reg_addr++;
            // Read register.
            node_status = NODE_read_register(uhfm_node, reg_addr, &(uhfm_registers[reg_addr]), &access_status);
            NODE_exit_error(RADIO_ERROR_BASE_NODE);
            // Check access status.
            if ((access_status.flags) != 0) {
                status = RADIO_ERROR_MODEM_DL_PAYLOAD;
                goto errors;
            }
        }
        // Convert to byte array.
        dl_payload[idx] = (uint8_t) ((uhfm_registers[reg_addr] >> (8 * (idx % 4))) & 0xFF);
    }
    (*dl_payload_available) = 1;
errors:
    return status;
}

/*******************************************************************/
RADIO_status_t RADIO_UHFM_get_last_bidirectional_mc(UNA_node_t* uhfm_node, uint32_t* last_bidirectional_mc) {
    // Local variables.
    RADIO_status_t status = RADIO_SUCCESS;
    NODE_status_t node_status = NODE_SUCCESS;
    UNA_access_status_t access_status;
    uint32_t reg_status_1 = 0;
    // Check parameters.
    if ((uhfm_node == NULL) || (last_bidirectional_mc == NULL)) {
        status = RADIO_ERROR_NULL_PARAMETER;
        goto errors;
    }
    // Read message status.
    node_status = NODE_read_register(uhfm_node, UHFM_REGISTER_ADDRESS_STATUS_1, &reg_status_1, &access_status);
    NODE_exit_error(RADIO_ERROR_BASE_NODE);
    // Check access status.
    if ((access_status.flags) != 0) {
        status = RADIO_ERROR_MODEM_DL_BIDIRECTIONAL_MC;
        goto errors;
    }
    // Compute message counter.
    (*last_bidirectional_mc) = SWREG_read_field(reg_status_1, UHFM_REGISTER_STATUS_1_MASK_BIDIRECTIONAL_MC);
errors:
    return status;
}
