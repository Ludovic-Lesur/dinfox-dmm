/*
 * uhfm.h
 *
 *  Created on: 14 feb. 2023
 *      Author: Ludo
 */

#ifndef __UHFM_H__
#define __UHFM_H__

#include "at_bus.h"
#include "common.h"
#include "node.h"
#include "string.h"
#include "types.h"
#include "uhfm_reg.h"

/*** UHFM macros ***/

#define UHFM_SIGFOX_UL_PAYLOAD_SIZE_MAX		12
#define UHFM_SIGFOX_DL_PAYLOAD_SIZE			8

/*** UHFM structures ***/

typedef enum {
	UHFM_LINE_DATA_INDEX_SIGFOX_EP_ID = COMMON_LINE_DATA_INDEX_LAST,
	UHFM_LINE_DATA_INDEX_VRF_TX,
	UHFM_LINE_DATA_INDEX_VRF_RX,
	UHFM_LINE_DATA_INDEX_LAST,
} UHFM_line_data_index_t;

typedef struct {
	uint8_t* ul_payload;
	uint8_t ul_payload_size;
	uint8_t bidirectional_flag;
} UHFM_sigfox_message_t;

static const uint32_t UHFM_REG_WRITE_TIMEOUT_MS[UHFM_NUMBER_OF_SPECIFIC_REG] = {
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS
};

/*** UHFM functions ***/

NODE_status_t UHFM_write_line_data(NODE_line_data_write_t* line_data_write, NODE_access_status_t* write_status);
NODE_status_t UHFM_read_line_data(NODE_line_data_read_t* line_data_read, NODE_access_status_t* read_status);
NODE_status_t UHFM_build_sigfox_ul_payload(NODE_ul_payload_update_t* ul_payload_update);

NODE_status_t UHFM_send_sigfox_message(NODE_address_t node_addr, UHFM_sigfox_message_t* sigfox_message, NODE_access_status_t* send_status);
NODE_status_t UHFM_get_dl_payload(NODE_address_t node_addr, uint8_t* dl_payload, NODE_access_status_t* read_status);

#endif /* __UHFM_H__ */
