/*
 * radio_r4s8cr.c
 *
 *  Created on: 05 feb. 2023
 *      Author: Ludo
 */

#include "radio_r4s8cr.h"

#include "common_registers.h"
#include "r4s8cr_registers.h"
#include "radio.h"
#include "radio_common.h"
#include "swreg.h"
#include "una.h"

/*** RADIO R4S8CR local macros ***/

#define R4S8CR_UL_PAYLOAD_ELECTRICAL_SIZE   2

/*** RADIO R4S8CR local structures ***/

/*******************************************************************/
typedef union {
    uint8_t frame[R4S8CR_UL_PAYLOAD_ELECTRICAL_SIZE];
    struct {
        unsigned r8stst :2;
        unsigned r7stst :2;
        unsigned r6stst :2;
        unsigned r5stst :2;
        unsigned r4stst :2;
        unsigned r3stst :2;
        unsigned r2stst :2;
        unsigned r1stst :2;
    } __attribute__((scalar_storage_order("big-endian")))__attribute__((packed));
} R4S8CR_ul_payload_data_t;

/*** RADIO R4S8CR local global variables ***/

static const uint8_t R4S8CR_REGISTER_LIST_UL_PAYLOAD_ELECTRICAL[] = {
    R4S8CR_REGISTER_ADDRESS_STATUS
};

/*** RADIO R4S8CR functions ***/

/*******************************************************************/
RADIO_status_t RADIO_R4S8CR_build_ul_node_payload(RADIO_node_t* radio_node, RADIO_ul_payload_t* node_payload) {
    // Local variables.
    RADIO_status_t status = RADIO_SUCCESS;
    NODE_status_t node_status = NODE_SUCCESS;
    uint32_t r4s8cr_registers[R4S8CR_REGISTER_ADDRESS_LAST];
    UNA_access_status_t access_status;
    R4S8CR_ul_payload_data_t ul_payload_data;
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
    for (idx = 0; idx < R4S8CR_REGISTER_ADDRESS_LAST; idx++) {
        r4s8cr_registers[idx] = R4S8CR_REGISTER_ERROR_VALUE[idx];
    }
    // Reset payload size.
    node_payload->payload_size = 0;
    // Read related registers.
    node_status = NODE_read_registers((radio_node->node), (uint8_t*) R4S8CR_REGISTER_LIST_UL_PAYLOAD_ELECTRICAL, sizeof(R4S8CR_REGISTER_LIST_UL_PAYLOAD_ELECTRICAL), (uint32_t*) r4s8cr_registers, &access_status);
    NODE_exit_error(RADIO_ERROR_BASE_NODE);
    // Build data payload.
    ul_payload_data.r1stst = SWREG_read_field(r4s8cr_registers[R4S8CR_REGISTER_ADDRESS_STATUS], R4S8CR_REGISTER_STATUS_MASK_R1STST);
    ul_payload_data.r2stst = SWREG_read_field(r4s8cr_registers[R4S8CR_REGISTER_ADDRESS_STATUS], R4S8CR_REGISTER_STATUS_MASK_R2STST);
    ul_payload_data.r3stst = SWREG_read_field(r4s8cr_registers[R4S8CR_REGISTER_ADDRESS_STATUS], R4S8CR_REGISTER_STATUS_MASK_R3STST);
    ul_payload_data.r4stst = SWREG_read_field(r4s8cr_registers[R4S8CR_REGISTER_ADDRESS_STATUS], R4S8CR_REGISTER_STATUS_MASK_R4STST);
    ul_payload_data.r5stst = SWREG_read_field(r4s8cr_registers[R4S8CR_REGISTER_ADDRESS_STATUS], R4S8CR_REGISTER_STATUS_MASK_R5STST);
    ul_payload_data.r6stst = SWREG_read_field(r4s8cr_registers[R4S8CR_REGISTER_ADDRESS_STATUS], R4S8CR_REGISTER_STATUS_MASK_R6STST);
    ul_payload_data.r7stst = SWREG_read_field(r4s8cr_registers[R4S8CR_REGISTER_ADDRESS_STATUS], R4S8CR_REGISTER_STATUS_MASK_R7STST);
    ul_payload_data.r8stst = SWREG_read_field(r4s8cr_registers[R4S8CR_REGISTER_ADDRESS_STATUS], R4S8CR_REGISTER_STATUS_MASK_R8STST);
    // Copy payload.
    for (idx = 0; idx < R4S8CR_UL_PAYLOAD_ELECTRICAL_SIZE; idx++) {
        (node_payload->payload)[idx] = ul_payload_data.frame[idx];
    }
    node_payload->payload_size = R4S8CR_UL_PAYLOAD_ELECTRICAL_SIZE;
errors:
    return status;
}
