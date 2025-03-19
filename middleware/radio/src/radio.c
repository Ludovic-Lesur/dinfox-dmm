/*
 * radio.c
 *
 *  Created on: 18 dec. 2024
 *      Author: Ludo
 */

#include "radio.h"

#include "error.h"
#include "error_base.h"
#include "dmm_registers.h"
#include "node.h"
#include "radio_bpsm.h"
#include "radio_common.h"
#include "radio_ddrm.h"
#include "radio_dmm.h"
#include "radio_gpsm.h"
#include "radio_lvrm.h"
#include "radio_mpmcm.h"
#include "radio_sm.h"
#include "radio_r4s8cr.h"
#include "radio_uhfm.h"
#include "rtc.h"
#include "swreg.h"
#include "types.h"
#include "uhfm_registers.h"
#include "una.h"

/*** RADIO local macros ***/

#define RADIO_UL_PAYLOAD_HEADER_SIZE_BYTES          2
#define RADIO_UL_NODE_PAYLOAD_MAX_SIZE_BYTES        (UHFM_UL_PAYLOAD_MAX_SIZE_BYTES - RADIO_UL_PAYLOAD_HEADER_SIZE_BYTES)
#define RADIO_UL_PAYLOAD_TYPE_COUNTER_ERROR_VALUE   0xFF

#define RADIO_DL_HASH_ERROR_VALUE                   0xFFFF
#define RADIO_DL_ACCESS_STATUS_ERROR_VALUE          0xFF

#define RADIO_ACTION_LIST_SIZE                      32

#define RADIO_UL_LOOP_MAX                           5

/*** RADIO local structures ***/

/*******************************************************************/
typedef union {
    uint8_t frame[UHFM_UL_PAYLOAD_MAX_SIZE_BYTES];
    struct {
        unsigned node_addr :8;
        unsigned board_id :8;
        uint8_t node_payload[RADIO_UL_NODE_PAYLOAD_MAX_SIZE_BYTES];
    } __attribute__((scalar_storage_order("big-endian")))__attribute__((packed));
} RADIO_ul_payload_t;

/*******************************************************************/
typedef RADIO_status_t (*RADIO_build_ul_node_payload_t)(RADIO_ul_node_payload_t* node_payload);

/*******************************************************************/
typedef struct {
    UNA_node_t* node;
    uint8_t payload_type_counter;
} RADIO_node_t;

/*******************************************************************/
typedef enum {
    RADIO_DL_OP_CODE_NOP = 0,
    RADIO_DL_OP_CODE_SINGLE_FULL_READ,
    RADIO_DL_OP_CODE_SINGLE_FULL_WRITE,
    RADIO_DL_OP_CODE_SINGLE_MASKED_WRITE,
    RADIO_DL_OP_CODE_TEMPORARY_FULL_WRITE,
    RADIO_DL_OP_CODE_TEMPORARY_MASKED_WRITE,
    RADIO_DL_OP_CODE_SUCCESSIVE_FULL_WRITE,
    RADIO_DL_OP_CODE_SUCCESSIVE_MASKED_WRITE,
    RADIO_DL_OP_CODE_DUAL_FULL_WRITE,
    RADIO_DL_OP_CODE_TRIPLE_FULL_WRITE,
    RADIO_DL_OP_CODE_DUAL_RADIO_WRITE,
    RADIO_DL_OP_CODE_LAST
} RADIO_dl_op_code_t;

/*******************************************************************/
typedef union {
    uint8_t frame[UHFM_DL_PAYLOAD_SIZE_BYTES];
    struct {
        unsigned op_code :8;
        union {
            struct {
                unsigned node_addr :8;
                unsigned reg_addr :8;
                unsigned unused_0 :32;
                unsigned unused_1 :8;
            } __attribute__((scalar_storage_order("big-endian")))__attribute__((packed)) single_full_read;
            struct {
                unsigned node_addr :8;
                unsigned reg_addr :8;
                unsigned reg_value :32;
                unsigned duration :8; // Unused in single.
            } __attribute__((scalar_storage_order("big-endian")))__attribute__((packed)) full_write;
            struct {
                unsigned node_addr :8;
                unsigned reg_addr :8;
                unsigned reg_mask :16;
                unsigned reg_value :16;
                unsigned duration :8; // Unused in single.
            } __attribute__((scalar_storage_order("big-endian")))__attribute__((packed)) masked_write;
            struct {
                unsigned node_addr :8;
                unsigned reg_addr :8;
                unsigned reg_value_1 :16;
                unsigned reg_value_2 :16;
                unsigned duration :8;
            } __attribute__((scalar_storage_order("big-endian")))__attribute__((packed)) successive_full_write;
            struct {
                unsigned node_addr :8;
                unsigned reg_addr :8;
                unsigned reg_mask :8;
                unsigned reg_value_1 :8;
                unsigned reg_value_2 :8;
                unsigned duration :8;
                unsigned unused :8;
            } __attribute__((scalar_storage_order("big-endian")))__attribute__((packed)) successive_masked_write;
            struct {
                unsigned node_addr :8;
                unsigned reg_1_addr :8;
                unsigned reg_1_value :16;
                unsigned reg_2_addr :8;
                unsigned reg_2_value :16;
            } __attribute__((scalar_storage_order("big-endian")))__attribute__((packed)) dual_full_write;
            struct {
                unsigned node_addr :8;
                unsigned reg_1_addr :8;
                unsigned reg_1_value :8;
                unsigned reg_2_addr :8;
                unsigned reg_2_value :8;
                unsigned reg_3_addr :8;
                unsigned reg_3_value :8;
            } __attribute__((scalar_storage_order("big-endian")))__attribute__((packed)) triple_full_write;
            struct {
                unsigned node_1_addr :8;
                unsigned reg_1_addr :8;
                unsigned reg_1_value :8;
                unsigned node_2_addr :8;
                unsigned reg_2_addr :8;
                unsigned reg_2_value :8;
                unsigned unused :8;
            } __attribute__((scalar_storage_order("big-endian")))__attribute__((packed)) dual_node_write;
        };
    } __attribute__((scalar_storage_order("big-endian")))__attribute__((packed));
} RADIO_dl_payload_t;

/*******************************************************************/
typedef struct {
    // Uplink.
    RADIO_node_t node_list[NODE_LIST_SIZE];
    uint32_t ul_next_time_seconds;
    uint8_t ul_node_list_index;
    // Downlink.
    RADIO_dl_payload_t dl_payload;
    uint32_t dl_next_time_seconds;
    // Node actions list.
    RADIO_node_action_t action[RADIO_ACTION_LIST_SIZE];
    // Specific nodes pointers.
    UNA_node_t* master_node_ptr;
    UNA_node_t* modem_node_ptr;
    UNA_node_t* mpmcm_node_ptr;
} RADIO_context_t;

/*** RADIO local global variables ***/

static const RADIO_build_ul_node_payload_t RADIO_NODE_DESCRIPTOR[UNA_BOARD_ID_LAST] = {
    &RADIO_LVRM_build_ul_node_payload,
    &RADIO_BPSM_build_ul_node_payload,
    &RADIO_DDRM_build_ul_node_payload,
    &RADIO_UHFM_build_ul_node_payload,
    &RADIO_GPSM_build_ul_node_payload,
    &RADIO_SM_build_ul_node_payload,
    NULL,
    NULL,
    &RADIO_DMM_build_ul_node_payload,
    &RADIO_MPMCM_build_ul_node_payload,
    &RADIO_R4S8CR_build_ul_node_payload
};

static RADIO_context_t radio_ctx = {
    .ul_next_time_seconds = 0,
    .ul_node_list_index = 0,
    .dl_next_time_seconds = 0,
    .master_node_ptr = NULL,
    .modem_node_ptr = NULL,
    .mpmcm_node_ptr = NULL
};

/*** RADIO local functions ***/

/*******************************************************************/
static void _RADIO_reset_node_list(RADIO_node_t* node_list) {
    // Local variables.
    uint8_t idx = 0;
    // Reset nodes list.
    for (idx = 0; idx < NODE_LIST_SIZE; idx++) {
        node_list[idx].node = NULL;
        node_list[idx].payload_type_counter = RADIO_UL_PAYLOAD_TYPE_COUNTER_ERROR_VALUE;
    }
}

/*******************************************************************/
static void _RADIO_synchronize_node_list(void) {
    // Local variables.
    RADIO_node_t tmp_node_list[NODE_LIST_SIZE];
    uint8_t new_idx = 0;
    uint8_t old_idx = 0;
    // Reset modem node pointer.
    radio_ctx.master_node_ptr = NULL;
    radio_ctx.modem_node_ptr = NULL;
    radio_ctx.mpmcm_node_ptr = NULL;
    // Reset temporary list.
    _RADIO_reset_node_list((RADIO_node_t*) tmp_node_list);
    // Copy all nodes from official list.
    for (new_idx = 0; new_idx < NODE_LIST.count; new_idx++) {
        // Update pointer.
        tmp_node_list[new_idx].node = &(NODE_LIST.list[new_idx]);
        tmp_node_list[new_idx].payload_type_counter = 0;
        // Restore previous payload type counters based on address.
        for (old_idx = 0; old_idx < NODE_LIST_SIZE; old_idx++) {
            // Check pointer.
            if (radio_ctx.node_list[old_idx].node != NULL) {
                // Compare address.
                if (((tmp_node_list[new_idx].node)->address) == ((radio_ctx.node_list[old_idx].node)->address)) {
                    tmp_node_list[new_idx].payload_type_counter = radio_ctx.node_list[old_idx].payload_type_counter;
                }
            }
        }
        // Update specific nodes pointer.
        if (NODE_LIST.list[new_idx].board_id == UNA_BOARD_ID_DMM) {
            radio_ctx.master_node_ptr = &(NODE_LIST.list[new_idx]);
        }
        if (NODE_LIST.list[new_idx].board_id == UNA_BOARD_ID_UHFM) {
            radio_ctx.modem_node_ptr = &(NODE_LIST.list[new_idx]);
        }
        if (NODE_LIST.list[new_idx].board_id == UNA_BOARD_ID_MPMCM) {
            radio_ctx.mpmcm_node_ptr = &(NODE_LIST.list[new_idx]);
        }
    }
    // Reset old list.
    _RADIO_reset_node_list((RADIO_node_t*) radio_ctx.node_list);
    // Update local list.
    for (new_idx = 0; new_idx < NODE_LIST.count; new_idx++) {
        radio_ctx.node_list[new_idx].node = tmp_node_list[new_idx].node;
        radio_ctx.node_list[new_idx].payload_type_counter = tmp_node_list[new_idx].payload_type_counter;
    }
}

/*******************************************************************/
static RADIO_status_t _RADIO_search_node(UNA_node_address_t node_addr, UNA_node_t** node_ptr) {
    // Local variables.
    RADIO_status_t status = RADIO_SUCCESS;
    uint8_t address_match = 0;
    uint8_t idx = 0;
    // Reset pointer.
    (*node_ptr) = NULL;
    // Search board in nodes list.
    for (idx = 0; idx < NODE_LIST.count; idx++) {
        // Compare address
        if (NODE_LIST.list[idx].address == node_addr) {
            address_match = 1;
            break;
        }
    }
    // Check flag.
    if (address_match == 0) {
        status = RADIO_ERROR_ACTION_NODE_ADDRESS;
        goto errors;
    }
    // Update pointer.
    (*node_ptr) = &NODE_LIST.list[idx];
errors:
    return status;
}

/*******************************************************************/
static RADIO_status_t _RADIO_transmit(RADIO_ul_node_payload_t* node_payload, uint8_t bidirectional_flag) {
    // Local variables.
    RADIO_status_t status = RADIO_SUCCESS;
    RADIO_ul_payload_t ul_payload;
    UHFM_ul_message_t uhfm_message;
    uint8_t idx = 0;
    // Check parameters.
    if (node_payload == NULL) {
        status = RADIO_ERROR_NULL_PARAMETER;
        goto errors;
    }
    if (((node_payload->node) == NULL) || (node_payload->payload == NULL)) {
        status = RADIO_ERROR_NULL_PARAMETER;
        goto errors;
    }
    if ((node_payload->payload_size) == 0) {
        status = RADIO_ERROR_UL_NODE_PAYLOAD_SIZE_UNDERFLOW;
        goto errors;
    }
    if ((node_payload->payload_size) > RADIO_UL_NODE_PAYLOAD_MAX_SIZE_BYTES) {
        status = RADIO_ERROR_UL_NODE_PAYLOAD_SIZE_OVERFLOW;
        goto errors;
    }
    // Check UHFM board availability.
    if (radio_ctx.modem_node_ptr == NULL) {
        status = RADIO_ERROR_MODEM_NODE_NOT_FOUND;
        goto errors;
    }
    // Reset payload.
    for (idx = 0; idx < UHFM_UL_PAYLOAD_MAX_SIZE_BYTES; idx++) {
        ul_payload.frame[idx] = 0x00;
    }
    // Add board ID and node address.
    ul_payload.board_id = (node_payload->node->board_id);
    ul_payload.node_addr = (node_payload->node->address);
    // Add node payload.
    for (idx = 0; idx < (node_payload->payload_size); idx++) {
        ul_payload.node_payload[idx] = (node_payload->payload)[idx];
    }
    // Build Sigfox message structure.
    uhfm_message.ul_payload = (uint8_t*) ul_payload.frame;
    uhfm_message.ul_payload_size = (RADIO_UL_PAYLOAD_HEADER_SIZE_BYTES + (node_payload->payload_size));
    uhfm_message.bidirectional_flag = bidirectional_flag;
    // Send message.
    status = RADIO_UHFM_send_ul_message(radio_ctx.modem_node_ptr, &uhfm_message);
    if (status != RADIO_SUCCESS) goto errors;
errors:
    return status;
}

/*******************************************************************/
static RADIO_status_t _RADIO_receive(void) {
    // Local variables.
    RADIO_status_t status = RADIO_SUCCESS;
    uint8_t dl_payload_available = 0;
    uint8_t dl_payload[UHFM_DL_PAYLOAD_SIZE_BYTES];
    uint8_t idx = 0;
    // Reset operation code.
    radio_ctx.dl_payload.op_code = RADIO_DL_OP_CODE_NOP;
    // Check UHFM board availability.
    if (radio_ctx.modem_node_ptr == NULL) {
        status = RADIO_ERROR_MODEM_NODE_NOT_FOUND;
        goto errors;
    }
    // Read downlink payload.
    status = RADIO_UHFM_get_dl_payload((radio_ctx.modem_node_ptr), &dl_payload_available, (uint8_t*) dl_payload);
    if (status != RADIO_SUCCESS) goto errors;
    // Update local buffer if new data is available.
    if (dl_payload_available != 0) {
        for (idx = 0; idx < UHFM_DL_PAYLOAD_SIZE_BYTES; idx++) {
            radio_ctx.dl_payload.frame[idx] = dl_payload[idx];
        }
    }
errors:
    return status;
}

/*******************************************************************/
static RADIO_status_t _RADIO_process_uplink(uint8_t bidirectional_flag, uint8_t* message_sent) {
    // Local variables.
    RADIO_status_t status = RADIO_SUCCESS;
    RADIO_ul_node_payload_t node_payload;
    uint8_t node_payload_bytes[RADIO_UL_NODE_PAYLOAD_MAX_SIZE_BYTES];
    uint8_t node_idx = radio_ctx.ul_node_list_index;
    uint8_t idx = 0;
    // Reset payload.
    for (idx = 0; idx < UHFM_UL_PAYLOAD_MAX_SIZE_BYTES; idx++) {
        node_payload_bytes[idx] = 0x00;
    }
    // Reset output flag.
    (*message_sent) = 0;
    // Build payload structure.
    node_payload.node = radio_ctx.node_list[node_idx].node;
    node_payload.payload = (uint8_t*) node_payload_bytes;
    node_payload.payload_size = 0;
    node_payload.payload_type_counter = radio_ctx.node_list[node_idx].payload_type_counter;
    // Execute function of the corresponding board ID.
    status = RADIO_NODE_DESCRIPTOR[(radio_ctx.node_list[node_idx].node)->board_id](&node_payload);
    if (status != RADIO_SUCCESS) goto errors;
    // Exit directly if node has no data to send.
    if (node_payload.payload_size == 0) goto errors;
    // Transmit message.
    status = _RADIO_transmit(&node_payload, bidirectional_flag);
    if (status != RADIO_SUCCESS) goto errors;
    // Update output flag.
    (*message_sent) = 1;
errors:
    radio_ctx.node_list[node_idx].payload_type_counter = node_payload.payload_type_counter;
    return status;
}

/*******************************************************************/
static RADIO_status_t _RADIO_record_action(RADIO_node_action_t* action) {
    // Local variables.
    RADIO_status_t status = RADIO_SUCCESS;
    uint8_t idx = 0;
    uint8_t slot_found = 0;
    // Check parameter.
    if (action == NULL) {
        status = RADIO_ERROR_NULL_PARAMETER;
        goto errors;
    }
    // Search available slot.
    for (idx = 0; idx < RADIO_ACTION_LIST_SIZE; idx++) {
        // Check node address.
        if (radio_ctx.action[idx].node == NULL) {
            // Store action.
            radio_ctx.action[idx].node = (action->node);
            radio_ctx.action[idx].downlink_hash = (action->downlink_hash);
            radio_ctx.action[idx].reg_addr = (action->reg_addr);
            radio_ctx.action[idx].reg_value = (action->reg_value);
            radio_ctx.action[idx].reg_mask = (action->reg_mask);
            radio_ctx.action[idx].timestamp_seconds = (action->timestamp_seconds);
            radio_ctx.action[idx].access_status = (action->access_status);
            // Update flag.
            slot_found = 1;
            break;
        }
    }
    // Check flag.
    if (slot_found == 0) {
        status = RADIO_ERROR_ACTION_LIST_OVERFLOW;
        goto errors;
    }
errors:
    return status;
}

/*******************************************************************/
static RADIO_status_t _RADIO_remove_action(uint8_t action_index) {
    // Local variables.
    RADIO_status_t status = RADIO_SUCCESS;
    // Check parameter.
    if (action_index >= RADIO_ACTION_LIST_SIZE) {
        status = RADIO_ERROR_ACTION_LIST_INDEX;
        goto errors;
    }
    radio_ctx.action[action_index].node = NULL;
    radio_ctx.action[action_index].downlink_hash = RADIO_DL_HASH_ERROR_VALUE;
    radio_ctx.action[action_index].reg_addr = 0x00;
    radio_ctx.action[action_index].reg_value = 0;
    radio_ctx.action[action_index].reg_mask = 0;
    radio_ctx.action[action_index].timestamp_seconds = 0;
    radio_ctx.action[action_index].access_status.all = RADIO_DL_ACCESS_STATUS_ERROR_VALUE;
errors:
    return status;
}

/*******************************************************************/
static RADIO_status_t _RADIO_process_downlink(void) {
    // Local variables.
    RADIO_status_t status = RADIO_SUCCESS;
    NODE_status_t node_status = NODE_SUCCESS;
    UNA_access_status_t access_status;
    RADIO_node_action_t action;
    uint32_t last_bidirectional_mc = 0;
    UNA_node_t* node_ptr = NULL;
    uint32_t previous_reg_value = 0;
    // Directly exit in case of NOP.
    if (radio_ctx.dl_payload.op_code == RADIO_DL_OP_CODE_NOP) goto errors;
    // Read last message counter.
    status = RADIO_UHFM_get_last_bidirectional_mc((radio_ctx.modem_node_ptr), &last_bidirectional_mc);
    if (status != RADIO_SUCCESS) goto errors;
    // Common action parameters.
    action.downlink_hash = last_bidirectional_mc;
    action.access_status.all = RADIO_DL_ACCESS_STATUS_ERROR_VALUE;
    // Check operation code.
    switch (radio_ctx.dl_payload.op_code) {
    case RADIO_DL_OP_CODE_NOP:
        // No operation.
        break;
    case RADIO_DL_OP_CODE_SINGLE_FULL_READ:
        // Search node.
        status = _RADIO_search_node(radio_ctx.dl_payload.single_full_read.node_addr, &node_ptr);
        if (status != RADIO_SUCCESS) goto errors;
        // Register action.
        action.node = node_ptr;
        action.access_status.type = UNA_ACCESS_TYPE_READ;
        action.reg_addr = radio_ctx.dl_payload.single_full_read.reg_addr;
        action.reg_value = 0;
        action.reg_mask = 0;
        action.timestamp_seconds = 0;
        status = _RADIO_record_action(&action);
        if (status != RADIO_SUCCESS) goto errors;
        break;
    case RADIO_DL_OP_CODE_SINGLE_FULL_WRITE:
        // Search node.
        status = _RADIO_search_node(radio_ctx.dl_payload.full_write.node_addr, &node_ptr);
        if (status != RADIO_SUCCESS) goto errors;
        // Register action.
        action.node = node_ptr;
        action.access_status.type = UNA_ACCESS_TYPE_WRITE;
        action.reg_addr = radio_ctx.dl_payload.full_write.reg_addr;
        action.reg_value = (uint32_t) radio_ctx.dl_payload.full_write.reg_value;
        action.reg_mask = UNA_REGISTER_MASK_ALL;
        action.timestamp_seconds = 0;
        status = _RADIO_record_action(&action);
        if (status != RADIO_SUCCESS) goto errors;
        break;
    case RADIO_DL_OP_CODE_SINGLE_MASKED_WRITE:
        // Search node.
        status = _RADIO_search_node(radio_ctx.dl_payload.masked_write.node_addr, &node_ptr);
        if (status != RADIO_SUCCESS) goto errors;
        // Register action.
        action.node = node_ptr;
        action.access_status.type = UNA_ACCESS_TYPE_WRITE;
        action.reg_addr = radio_ctx.dl_payload.masked_write.reg_addr;
        action.reg_value = (uint32_t) radio_ctx.dl_payload.masked_write.reg_value;
        action.reg_mask = (uint32_t) radio_ctx.dl_payload.masked_write.reg_mask;
        action.timestamp_seconds = 0;
        status = _RADIO_record_action(&action);
        if (status != RADIO_SUCCESS) goto errors;
        break;
    case RADIO_DL_OP_CODE_TEMPORARY_FULL_WRITE:
        // Search node.
        status = _RADIO_search_node(radio_ctx.dl_payload.full_write.node_addr, &node_ptr);
        if (status != RADIO_SUCCESS) goto errors;
        // Read current register value.
        node_status = NODE_read_register(node_ptr, radio_ctx.dl_payload.full_write.reg_addr, &previous_reg_value, &access_status);
        NODE_exit_error(RADIO_ERROR_BASE_NODE);
        // Check access status.
        if (access_status.flags != 0) {
            status = RADIO_ERROR_ACTION_READ_ACCESS;
            goto errors;
        }
        // Register first action.
        action.node = node_ptr;
        action.access_status.type = UNA_ACCESS_TYPE_WRITE;
        action.reg_addr = radio_ctx.dl_payload.full_write.reg_addr;
        action.reg_value = (uint32_t) radio_ctx.dl_payload.full_write.reg_value;
        action.reg_mask = UNA_REGISTER_MASK_ALL;
        action.timestamp_seconds = 0;
        status = _RADIO_record_action(&action);
        if (status != RADIO_SUCCESS) goto errors;
        // Register second action.
        action.reg_value = previous_reg_value;
        action.timestamp_seconds = RTC_get_uptime_seconds() + UNA_get_seconds(radio_ctx.dl_payload.full_write.duration);
        status = _RADIO_record_action(&action);
        if (status != RADIO_SUCCESS) goto errors;
        break;
    case RADIO_DL_OP_CODE_TEMPORARY_MASKED_WRITE:
        // Search node.
        status = _RADIO_search_node(radio_ctx.dl_payload.masked_write.node_addr, &node_ptr);
        if (status != RADIO_SUCCESS) goto errors;
        // Read current register value.
        node_status = NODE_read_register(node_ptr, radio_ctx.dl_payload.masked_write.reg_addr, &previous_reg_value, &access_status);
        NODE_exit_error(RADIO_ERROR_BASE_NODE);
        // Check access status.
        if (access_status.flags != 0) {
            status = RADIO_ERROR_ACTION_READ_ACCESS;
            goto errors;
        }
        // Register first action.
        action.node = node_ptr;
        action.access_status.type = UNA_ACCESS_TYPE_WRITE;
        action.reg_addr = radio_ctx.dl_payload.masked_write.reg_addr;
        action.reg_value = (uint32_t) radio_ctx.dl_payload.masked_write.reg_value;
        action.reg_mask = (uint32_t) radio_ctx.dl_payload.masked_write.reg_mask;
        action.timestamp_seconds = 0;
        status = _RADIO_record_action(&action);
        if (status != RADIO_SUCCESS) goto errors;
        // Register second action.
        action.reg_value = previous_reg_value;
        action.timestamp_seconds = RTC_get_uptime_seconds() + UNA_get_seconds(radio_ctx.dl_payload.masked_write.duration);
        status = _RADIO_record_action(&action);
        if (status != RADIO_SUCCESS) goto errors;
        break;
    case RADIO_DL_OP_CODE_SUCCESSIVE_FULL_WRITE:
        // Search node.
        status = _RADIO_search_node(radio_ctx.dl_payload.successive_full_write.node_addr, &node_ptr);
        if (status != RADIO_SUCCESS) goto errors;
        // Register first action.
        action.node = node_ptr;
        action.access_status.type = UNA_ACCESS_TYPE_WRITE;
        action.reg_addr = radio_ctx.dl_payload.successive_full_write.reg_addr;
        action.reg_value = (uint32_t) radio_ctx.dl_payload.successive_full_write.reg_value_1;
        action.reg_mask = UNA_REGISTER_MASK_ALL;
        action.timestamp_seconds = 0;
        status = _RADIO_record_action(&action);
        if (status != RADIO_SUCCESS) goto errors;
        // Register second action.
        action.reg_value = (uint32_t) radio_ctx.dl_payload.successive_full_write.reg_value_2;
        action.timestamp_seconds = RTC_get_uptime_seconds() + UNA_get_seconds(radio_ctx.dl_payload.successive_full_write.duration);
        status = _RADIO_record_action(&action);
        if (status != RADIO_SUCCESS) goto errors;
        break;
    case RADIO_DL_OP_CODE_SUCCESSIVE_MASKED_WRITE:
        // Search node.
        status = _RADIO_search_node(radio_ctx.dl_payload.successive_masked_write.node_addr, &node_ptr);
        if (status != RADIO_SUCCESS) goto errors;
        // Register first action.
        action.node = node_ptr;
        action.access_status.type = UNA_ACCESS_TYPE_WRITE;
        action.reg_addr = radio_ctx.dl_payload.successive_masked_write.reg_addr;
        action.reg_value = (uint32_t) radio_ctx.dl_payload.successive_masked_write.reg_value_1;
        action.reg_mask = (uint32_t) radio_ctx.dl_payload.successive_masked_write.reg_mask;
        action.timestamp_seconds = 0;
        status = _RADIO_record_action(&action);
        if (status != RADIO_SUCCESS) goto errors;
        // Register second action.
        action.reg_value = (uint32_t) radio_ctx.dl_payload.successive_masked_write.reg_value_2;
        action.timestamp_seconds = RTC_get_uptime_seconds() + UNA_get_seconds(radio_ctx.dl_payload.successive_masked_write.duration);
        status = _RADIO_record_action(&action);
        if (status != RADIO_SUCCESS) goto errors;
        break;
    case RADIO_DL_OP_CODE_DUAL_FULL_WRITE:
        // Search node.
        status = _RADIO_search_node(radio_ctx.dl_payload.dual_full_write.node_addr, &node_ptr);
        if (status != RADIO_SUCCESS) goto errors;
        // Register first action.
        action.node = node_ptr;
        action.access_status.type = UNA_ACCESS_TYPE_WRITE;
        action.reg_addr = radio_ctx.dl_payload.dual_full_write.reg_1_addr;
        action.reg_value = (uint32_t) radio_ctx.dl_payload.dual_full_write.reg_1_value;
        action.reg_mask = UNA_REGISTER_MASK_ALL;
        action.timestamp_seconds = 0;
        status = _RADIO_record_action(&action);
        if (status != RADIO_SUCCESS) goto errors;
        // Register second action.
        action.reg_addr = radio_ctx.dl_payload.dual_full_write.reg_2_addr;
        action.reg_value = (uint32_t) radio_ctx.dl_payload.dual_full_write.reg_2_value;
        status = _RADIO_record_action(&action);
        if (status != RADIO_SUCCESS) goto errors;
        break;
    case RADIO_DL_OP_CODE_TRIPLE_FULL_WRITE:
        // Search node.
        status = _RADIO_search_node(radio_ctx.dl_payload.triple_full_write.node_addr, &node_ptr);
        if (status != RADIO_SUCCESS) goto errors;
        // Register first action.
        action.node = node_ptr;
        action.access_status.type = UNA_ACCESS_TYPE_WRITE;
        action.reg_addr = radio_ctx.dl_payload.triple_full_write.reg_1_addr;
        action.reg_value = (uint32_t) radio_ctx.dl_payload.triple_full_write.reg_1_value;
        action.reg_mask = UNA_REGISTER_MASK_ALL;
        action.timestamp_seconds = 0;
        status = _RADIO_record_action(&action);
        if (status != RADIO_SUCCESS) goto errors;
        // Register second action.
        action.reg_addr = radio_ctx.dl_payload.triple_full_write.reg_2_addr;
        action.reg_value = (uint32_t) radio_ctx.dl_payload.triple_full_write.reg_2_value;
        status = _RADIO_record_action(&action);
        if (status != RADIO_SUCCESS) goto errors;
        // Register third action.
        action.reg_addr = radio_ctx.dl_payload.triple_full_write.reg_3_addr;
        action.reg_value = (uint32_t) radio_ctx.dl_payload.triple_full_write.reg_3_value;
        status = _RADIO_record_action(&action);
        if (status != RADIO_SUCCESS) goto errors;
        break;
    case RADIO_DL_OP_CODE_DUAL_RADIO_WRITE:
        // Search first node.
        status = _RADIO_search_node(radio_ctx.dl_payload.dual_node_write.node_1_addr, &node_ptr);
        if (status != RADIO_SUCCESS) goto errors;
        // Register first action.
        action.node = node_ptr;
        action.access_status.type = UNA_ACCESS_TYPE_WRITE;
        action.reg_addr = radio_ctx.dl_payload.dual_node_write.reg_1_addr;
        action.reg_value = (uint32_t) radio_ctx.dl_payload.dual_node_write.reg_1_value;
        action.reg_mask = UNA_REGISTER_MASK_ALL;
        action.timestamp_seconds = 0;
        status = _RADIO_record_action(&action);
        if (status != RADIO_SUCCESS) goto errors;
        // Search second node.
        status = _RADIO_search_node(radio_ctx.dl_payload.dual_node_write.node_2_addr, &node_ptr);
        if (status != RADIO_SUCCESS) goto errors;
        // Register second action.
        action.node = node_ptr;
        action.reg_addr = radio_ctx.dl_payload.dual_node_write.reg_2_addr;
        action.reg_value = (uint32_t) radio_ctx.dl_payload.dual_node_write.reg_2_value;
        status = _RADIO_record_action(&action);
        if (status != RADIO_SUCCESS) goto errors;
        break;
    default:
        status = RADIO_ERROR_DL_OPERATION_CODE;
        break;
    }
errors:
    return status;
}

/*******************************************************************/
static RADIO_status_t _RADIO_execute_actions(void) {
    // Local variables.
    RADIO_status_t status = RADIO_SUCCESS;
    NODE_status_t node_status = NODE_SUCCESS;
    uint8_t node_payload_bytes[RADIO_UL_NODE_PAYLOAD_MAX_SIZE_BYTES];
    RADIO_ul_node_payload_t node_payload;
    RADIO_node_action_t node_action;
    uint8_t idx = 0;
    // Loop on action table.
    for (idx = 0; idx < RADIO_ACTION_LIST_SIZE; idx++) {
        // Check NODE pointer and timestamp.
        if ((radio_ctx.action[idx].node != NULL) && (RTC_get_uptime_seconds() >= radio_ctx.action[idx].timestamp_seconds)) {
            // Copy action locally.
            node_action.node = radio_ctx.action[idx].node;
            node_action.downlink_hash = radio_ctx.action[idx].downlink_hash;
            node_action.timestamp_seconds = radio_ctx.action[idx].timestamp_seconds;
            node_action.reg_addr = radio_ctx.action[idx].reg_addr;
            node_action.reg_mask = radio_ctx.action[idx].reg_mask;
            node_action.reg_value = radio_ctx.action[idx].reg_value;
            node_action.access_status = radio_ctx.action[idx].access_status;
            // Remove action before execution.
            status = _RADIO_remove_action(idx);
            if (status != RADIO_SUCCESS) goto errors;
            // Turn bus interface on.
            POWER_enable(POWER_REQUESTER_ID_RADIO, POWER_DOMAIN_RS485, LPTIM_DELAY_MODE_STOP);
            // Perform node access (status is not checked because action log message must be sent whatever the result).
            if (node_action.access_status.type == UNA_ACCESS_TYPE_WRITE) {
                node_status = NODE_write_register(node_action.node, node_action.reg_addr, node_action.reg_value, node_action.reg_mask, &(node_action.access_status));
                NODE_exit_error(RADIO_ERROR_BASE_NODE);
            }
            else {
                node_status = NODE_read_register(node_action.node, node_action.reg_addr, &(node_action.reg_value), &(node_action.access_status));
                NODE_exit_error(RADIO_ERROR_BASE_NODE);
            }
            // Build payload structure.
            node_payload.node = node_action.node;
            node_payload.payload = (uint8_t*) node_payload_bytes;
            node_payload.payload_size = 0;
            node_payload.payload_type_counter = 0;
            // Build frame.
            status = RADIO_COMMON_build_ul_node_payload_action_log(&node_payload, &node_action);
            if (status != RADIO_SUCCESS) goto errors;
            // Send action log message.
            status = _RADIO_transmit(&node_payload, 0);
            if (status != RADIO_SUCCESS) goto errors;
        }
    }
errors:
    return status;
}

/*** RADIO functions ***/

/*******************************************************************/
RADIO_status_t RADIO_init(void) {
    // Local variables.
    RADIO_status_t status = RADIO_SUCCESS;
    uint8_t idx = 0;
    // Init context.
    radio_ctx.ul_next_time_seconds = 0;
    radio_ctx.ul_node_list_index = 0;
    radio_ctx.dl_next_time_seconds = 0;
    for (idx = 0; idx < UHFM_DL_PAYLOAD_SIZE_BYTES; idx++) {
        radio_ctx.dl_payload.frame[idx] = 0;
    }
    // Reset nodes list.
    _RADIO_reset_node_list((RADIO_node_t*) radio_ctx.node_list);
    radio_ctx.master_node_ptr = NULL;
    radio_ctx.modem_node_ptr = NULL;
    radio_ctx.mpmcm_node_ptr = NULL;
    // Reset actions list.
    for (idx = 0; idx < RADIO_ACTION_LIST_SIZE; idx++) {
        status = _RADIO_remove_action(idx);
        if (status != RADIO_SUCCESS) goto errors;
    }
errors:
    return status;
}

/*******************************************************************/
RADIO_status_t RADIO_de_init(void) {
    // Local variables.
    RADIO_status_t status = RADIO_SUCCESS;
    return status;
}

/*******************************************************************/
RADIO_status_t RADIO_process(void) {
    // Local variables.
    RADIO_status_t status = RADIO_SUCCESS;
    RADIO_status_t radio_status = RADIO_SUCCESS;
    NODE_status_t node_status = NODE_SUCCESS;
    UNA_access_status_t read_status;
    uint8_t message_sent = 0;
    uint32_t reg_value = 0;
    uint8_t bidirectional_flag = 0;
    uint8_t ul_next_time_update_required = 0;
    uint8_t dl_next_time_update_required = 0;
    uint8_t ul_loop = 0;
    // Check uplink period.
    if (RTC_get_uptime_seconds() >= radio_ctx.ul_next_time_seconds) {
        // Next time update needed.
        ul_next_time_update_required = 1;
        // Synchronize nodes list.
        _RADIO_synchronize_node_list();
        // Directly exit if there is no modem.
        if (radio_ctx.modem_node_ptr == NULL) goto errors;
        // Check downlink period.
        if (RTC_get_uptime_seconds() >= radio_ctx.dl_next_time_seconds) {
            // Next time update needed and set bidirectional flag.
            dl_next_time_update_required = 1;
            bidirectional_flag = 1;
        }
        // Turn bus interface on.
        POWER_enable(POWER_REQUESTER_ID_RADIO, POWER_DOMAIN_RS485, LPTIM_DELAY_MODE_STOP);
        // Set radio times to now to compensate node update duration.
        if (ul_next_time_update_required != 0) {
            radio_ctx.ul_next_time_seconds = RTC_get_uptime_seconds();
        }
        if (dl_next_time_update_required != 0) {
            radio_ctx.dl_next_time_seconds = RTC_get_uptime_seconds();
        }
        // Process MPMCM is needed.
        if ((radio_ctx.mpmcm_node_ptr != NULL) && (radio_ctx.modem_node_ptr != NULL)) {
            radio_status = RADIO_MPMCM_process(radio_ctx.mpmcm_node_ptr, &_RADIO_transmit);
            RADIO_stack_error(ERROR_BASE_RADIO);
        }
        do {
            // Send data through radio.
            radio_status = _RADIO_process_uplink(bidirectional_flag, &message_sent);
            RADIO_stack_error(ERROR_BASE_RADIO);
            // Switch to next node.
            radio_ctx.ul_node_list_index++;
            if (radio_ctx.ul_node_list_index >= NODE_LIST.count) {
                // Come back to first node.
                radio_ctx.ul_node_list_index = 0;
            }
            // Manage loop timeout.
            ul_loop++;
            if (ul_loop > RADIO_UL_LOOP_MAX) break;
        }
        while (message_sent == 0);
        // Execute downlink operation if needed.
        if (bidirectional_flag != 0) {
            // Read downlink payload.
            radio_status = _RADIO_receive();
            RADIO_stack_error(ERROR_BASE_RADIO);
            // Decode downlink payload is successfully read.
            if (radio_status == RADIO_SUCCESS) {
                radio_status = _RADIO_process_downlink();
                RADIO_stack_error(ERROR_BASE_RADIO);
            }
        }
    }
    // Execute actions.
    radio_status = _RADIO_execute_actions();
    RADIO_stack_error(ERROR_BASE_RADIO);
errors:
    // Update next radio times.
    node_status = NODE_read_register(radio_ctx.master_node_ptr, DMM_REGISTER_ADDRESS_CONFIGURATION_0, &reg_value, &read_status);
    NODE_stack_error(ERROR_BASE_NODE);
    // This is done here in case the downlink modified one of the periods (in order to take it into account directly for next radio wake-up).
    if (ul_next_time_update_required != 0) {
        radio_ctx.ul_next_time_seconds += UNA_get_seconds((uint32_t) SWREG_read_field(reg_value, DMM_REGISTER_CONFIGURATION_0_MASK_UL_PERIOD));
    }
    if (dl_next_time_update_required != 0) {
        radio_ctx.dl_next_time_seconds += UNA_get_seconds((uint32_t) SWREG_read_field(reg_value, DMM_REGISTER_CONFIGURATION_0_MASK_DL_PERIOD));
    }
    // Turn bus interface off.
    POWER_disable(POWER_REQUESTER_ID_RADIO, POWER_DOMAIN_RS485);
    return status;
}
