/*
 * radio_sm.c
 *
 *  Created on: 04 mar. 2023
 *      Author: Ludo
 */

#include "radio_sm.h"

#include "common_registers.h"
#include "radio.h"
#include "radio_common.h"
#include "sm_registers.h"
#include "swreg.h"
#include "una.h"

/*** RADIO SM local macros ***/

#define RADIO_SM_UL_PAYLOAD_MONITORING_SIZE     3
#define RADIO_SM_UL_PAYLOAD_ELECTRICAL_SIZE     9
#define RADIO_SM_UL_PAYLOAD_SENSOR_SIZE         2

#define RADIO_SM_UL_PAYLOAD_LOOP_MAX            5

/*** RADIO SM local structures ***/

/*******************************************************************/
typedef enum {
    RADIO_SM_UL_PAYLOAD_TYPE_MONITORING = 0,
    RADIO_SM_UL_PAYLOAD_TYPE_ELECTRICAL,
    RADIO_SM_UL_PAYLOAD_TYPE_SENSOR,
    RADIO_SM_UL_PAYLOAD_TYPE_LAST
} RADIO_SM_ul_payload_type_t;

/*******************************************************************/
typedef union {
    uint8_t frame[RADIO_SM_UL_PAYLOAD_MONITORING_SIZE];
    struct {
        unsigned vmcu :16;
        unsigned tmcu :8;
    } __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} RADIO_SM_ul_payload_monitoring_t;

/*******************************************************************/
typedef union {
    uint8_t frame[RADIO_SM_UL_PAYLOAD_ELECTRICAL_SIZE];
    struct {
        unsigned ain0 :16;
        unsigned ain1 :16;
        unsigned ain2 :16;
        unsigned ain3 :16;
        unsigned dio3 :2;
        unsigned dio2 :2;
        unsigned dio1 :2;
        unsigned dio0 :2;
    } __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} RADIO_SM_ul_payload_electrical_t;

/*******************************************************************/
typedef union {
    uint8_t frame[RADIO_SM_UL_PAYLOAD_SENSOR_SIZE];
    struct {
        unsigned tamb :8;
        unsigned hamb :8;
    } __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} RADIO_SM_ul_payload_sensor_t;

/*** SM local global variables ***/

static const uint8_t RADIO_SM_REGISTERS_MONITORING[] = {
    COMMON_REGISTER_ADDRESS_ANALOG_DATA_0
};

static const uint8_t RADIO_SM_REGISTERS_ELECTRICAL[] = {
    SM_REGISTER_ADDRESS_ANALOG_DATA_1,
    SM_REGISTER_ADDRESS_ANALOG_DATA_2,
    SM_REGISTER_ADDRESS_DIGITAL_DATA
};

static const uint8_t RADIO_SM_REGISTERS_SENSOR[] = {
    SM_REGISTER_ADDRESS_ANALOG_DATA_3
};

static const RADIO_SM_ul_payload_type_t RADIO_SM_UL_PAYLOAD_PATTERN[] = {
    RADIO_SM_UL_PAYLOAD_TYPE_ELECTRICAL,
    RADIO_SM_UL_PAYLOAD_TYPE_SENSOR,
    RADIO_SM_UL_PAYLOAD_TYPE_ELECTRICAL,
    RADIO_SM_UL_PAYLOAD_TYPE_SENSOR,
    RADIO_SM_UL_PAYLOAD_TYPE_ELECTRICAL,
    RADIO_SM_UL_PAYLOAD_TYPE_SENSOR,
    RADIO_SM_UL_PAYLOAD_TYPE_ELECTRICAL,
    RADIO_SM_UL_PAYLOAD_TYPE_SENSOR,
    RADIO_SM_UL_PAYLOAD_TYPE_ELECTRICAL,
    RADIO_SM_UL_PAYLOAD_TYPE_SENSOR,
    RADIO_SM_UL_PAYLOAD_TYPE_MONITORING
};

/*** RADIO SM functions ***/

/*******************************************************************/
RADIO_status_t RADIO_SM_build_ul_node_payload(RADIO_node_t* radio_node, RADIO_ul_payload_t* node_payload) {
    // Local variables.
    RADIO_status_t status = RADIO_SUCCESS;
    NODE_status_t node_status = NODE_SUCCESS;
    UNA_access_status_t access_status;
    uint32_t sm_registers[SM_REGISTER_ADDRESS_LAST];
    RADIO_SM_ul_payload_monitoring_t ul_payload_monitoring;
    RADIO_SM_ul_payload_electrical_t ul_payload_electrical;
    RADIO_SM_ul_payload_sensor_t ul_payload_sensor;
    uint32_t reg_configuration = 0;
    uint8_t idx = 0;
    uint32_t loop_count = 0;
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
    for (idx = 0; idx < SM_REGISTER_ADDRESS_LAST; idx++) {
        sm_registers[idx] = SM_REGISTER[idx].error_value;
    }
    // Reset payload size.
    node_payload->payload_size = 0;
    // Check event driven payloads.
    status = RADIO_COMMON_check_event_driven_payloads(radio_node, node_payload, (uint32_t*) sm_registers);
    if (status != RADIO_SUCCESS) goto errors;
    // Directly exits if a common payload was computed.
    if ((node_payload->payload_size) > 0) goto errors;
    // Else use specific pattern of the node.
    node_status = NODE_read_register((radio_node->node), SM_REGISTER_ADDRESS_FLAGS_1, &(sm_registers[SM_REGISTER_ADDRESS_FLAGS_1]), &access_status);
    NODE_exit_error(RADIO_ERROR_BASE_NODE);
    if (access_status.flags != 0) goto errors;
    // Update local value.
    reg_configuration = sm_registers[SM_REGISTER_ADDRESS_FLAGS_1];
    // Payloads loop.
    do {
        switch (RADIO_SM_UL_PAYLOAD_PATTERN[radio_node->payload_type_counter]) {
        case RADIO_SM_UL_PAYLOAD_TYPE_MONITORING:
            // Perform measurements.
            node_status = NODE_perform_measurements((radio_node->node), &access_status);
            NODE_exit_error(RADIO_ERROR_BASE_NODE);
            // Check write status.
            if (access_status.flags == 0) {
                // Read related registers.
                node_status = NODE_read_registers((radio_node->node), (uint8_t*) RADIO_SM_REGISTERS_MONITORING, sizeof(RADIO_SM_REGISTERS_MONITORING), (uint32_t*) sm_registers, &access_status);
                NODE_exit_error(RADIO_ERROR_BASE_NODE);
            }
            // Build data payload.
            ul_payload_monitoring.vmcu = SWREG_read_field(sm_registers[COMMON_REGISTER_ADDRESS_ANALOG_DATA_0], COMMON_REGISTER_ANALOG_DATA_0_MASK_VMCU);
            ul_payload_monitoring.tmcu = (SWREG_read_field(sm_registers[COMMON_REGISTER_ADDRESS_ANALOG_DATA_0], COMMON_REGISTER_ANALOG_DATA_0_MASK_TMCU) / 10);
            // Copy payload.
            for (idx = 0; idx < RADIO_SM_UL_PAYLOAD_MONITORING_SIZE; idx++) {
                (node_payload->payload)[idx] = ul_payload_monitoring.frame[idx];
            }
            node_payload->payload_size = RADIO_SM_UL_PAYLOAD_MONITORING_SIZE;
            break;
        case RADIO_SM_UL_PAYLOAD_TYPE_ELECTRICAL:
            // Check compilation flags.
            if (((reg_configuration & SM_REGISTER_FLAGS_1_MASK_AINF) == 0) && ((reg_configuration & SM_REGISTER_FLAGS_1_MASK_DIOF) == 0)) break;
            // Perform measurements.
            node_status = NODE_perform_measurements((radio_node->node), &access_status);
            NODE_exit_error(RADIO_ERROR_BASE_NODE);
            // Check write status.
            if (access_status.flags == 0) {
                // Read related registers.
                node_status = NODE_read_registers((radio_node->node), (uint8_t*) RADIO_SM_REGISTERS_ELECTRICAL, sizeof(RADIO_SM_REGISTERS_ELECTRICAL), (uint32_t*) sm_registers, &access_status);
                NODE_exit_error(RADIO_ERROR_BASE_NODE);
            }
            // Build data payload.
            ul_payload_electrical.ain0 = SWREG_read_field(sm_registers[SM_REGISTER_ADDRESS_ANALOG_DATA_1], SM_REGISTER_ANALOG_DATA_1_MASK_VAIN0);
            ul_payload_electrical.ain1 = SWREG_read_field(sm_registers[SM_REGISTER_ADDRESS_ANALOG_DATA_1], SM_REGISTER_ANALOG_DATA_1_MASK_VAIN1);
            ul_payload_electrical.ain2 = SWREG_read_field(sm_registers[SM_REGISTER_ADDRESS_ANALOG_DATA_2], SM_REGISTER_ANALOG_DATA_2_MASK_VAIN2);
            ul_payload_electrical.ain3 = SWREG_read_field(sm_registers[SM_REGISTER_ADDRESS_ANALOG_DATA_2], SM_REGISTER_ANALOG_DATA_2_MASK_VAIN3);
            ul_payload_electrical.dio0 = SWREG_read_field(sm_registers[SM_REGISTER_ADDRESS_DIGITAL_DATA], SM_REGISTER_DIGITAL_DATA_MASK_DIO0);
            ul_payload_electrical.dio1 = SWREG_read_field(sm_registers[SM_REGISTER_ADDRESS_DIGITAL_DATA], SM_REGISTER_DIGITAL_DATA_MASK_DIO1);
            ul_payload_electrical.dio2 = SWREG_read_field(sm_registers[SM_REGISTER_ADDRESS_DIGITAL_DATA], SM_REGISTER_DIGITAL_DATA_MASK_DIO2);
            ul_payload_electrical.dio3 = SWREG_read_field(sm_registers[SM_REGISTER_ADDRESS_DIGITAL_DATA], SM_REGISTER_DIGITAL_DATA_MASK_DIO3);
            // Copy payload.
            for (idx = 0; idx < RADIO_SM_UL_PAYLOAD_ELECTRICAL_SIZE; idx++) {
                (node_payload->payload)[idx] = ul_payload_electrical.frame[idx];
            }
            node_payload->payload_size = RADIO_SM_UL_PAYLOAD_ELECTRICAL_SIZE;
            break;
        case RADIO_SM_UL_PAYLOAD_TYPE_SENSOR:
            // Check compilation flags.
            if ((reg_configuration & SM_REGISTER_FLAGS_1_MASK_DIGF) == 0) break;
            // Perform measurements.
            node_status = NODE_perform_measurements((radio_node->node), &access_status);
            NODE_exit_error(RADIO_ERROR_BASE_NODE);
            // Check write status.
            if (access_status.flags == 0) {
                // Read related registers.
                node_status = NODE_read_registers((radio_node->node), (uint8_t*) RADIO_SM_REGISTERS_SENSOR, sizeof(RADIO_SM_REGISTERS_SENSOR), (uint32_t*) sm_registers, &access_status);
                NODE_exit_error(RADIO_ERROR_BASE_NODE);
            }
            // Build data payload.
            ul_payload_sensor.tamb = (SWREG_read_field(sm_registers[SM_REGISTER_ADDRESS_ANALOG_DATA_3], SM_REGISTER_ANALOG_DATA_3_MASK_TAMB) / 10);
            ul_payload_sensor.hamb = SWREG_read_field(sm_registers[SM_REGISTER_ADDRESS_ANALOG_DATA_3], SM_REGISTER_ANALOG_DATA_3_MASK_HAMB);
            // Copy payload.
            for (idx = 0; idx < RADIO_SM_UL_PAYLOAD_SENSOR_SIZE; idx++) {
                (node_payload->payload)[idx] = ul_payload_sensor.frame[idx];
            }
            node_payload->payload_size = RADIO_SM_UL_PAYLOAD_SENSOR_SIZE;
            break;
        default:
            status = RADIO_ERROR_UL_NODE_PAYLOAD_TYPE;
            goto errors;
        }
        // Increment payload type counter.
        radio_node->payload_type_counter = (((radio_node->payload_type_counter) + 1) % sizeof(RADIO_SM_UL_PAYLOAD_PATTERN));
        // Exit in case of loop error.
        loop_count++;
        if (loop_count > RADIO_SM_UL_PAYLOAD_LOOP_MAX) goto errors;
    }
    while ((node_payload->payload_size) == 0);
errors:
    return status;
}
