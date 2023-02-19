/*
 * uhfm.h
 *
 *  Created on: 14 feb. 2023
 *      Author: Ludo
 */

#ifndef __UHFM_H__
#define __UHFM_H__

#include "dinfox.h"
#include "node.h"
#include "string.h"
#include "types.h"

/*** UHFM structures ***/

typedef enum {
	UHFM_REGISTER_VRF_MV = DINFOX_REGISTER_LAST,
	UHFM_REGISTER_LAST,
} UHFM_register_address_t;

typedef enum {
	UHFM_STRING_DATA_INDEX_VRF_MV = DINFOX_STRING_DATA_INDEX_LAST,
	UHFM_STRING_DATA_INDEX_LAST,
} UHFM_string_data_index_t;

typedef struct {
	uint8_t* ul_payload;
	uint8_t ul_payload_size;
	uint8_t bidirectional_flag;
	uint8_t* dl_payload;
} UHFM_sigfox_message_t;

/*** UHFM macros ***/

#define UHFM_NUMBER_OF_SPECIFIC_REGISTERS	(UHFM_REGISTER_LAST - DINFOX_REGISTER_LAST)
#define UHFM_NUMBER_OF_SPECIFIC_STRING_DATA	(UHFM_STRING_DATA_INDEX_LAST - DINFOX_STRING_DATA_INDEX_LAST)

static const STRING_format_t UHFM_REGISTERS_FORMAT[UHFM_NUMBER_OF_SPECIFIC_REGISTERS] = {
	STRING_FORMAT_DECIMAL,
};

/*** UHFM functions ***/

NODE_status_t UHFM_update_data(NODE_data_update_t* data_update);
NODE_status_t UHFM_get_sigfox_payload(int32_t* integer_data_value, NODE_sigfox_payload_type_t sigfox_payload_type, uint8_t* sigfox_payload, uint8_t* sigfox_payload_size);
#ifdef AM
NODE_status_t UHFM_send_sigfox_message(NODE_address_t node_address, UHFM_sigfox_message_t* sigfox_message, NODE_access_status_t* send_status);
#else
NODE_status_t UHFM_send_sigfox_message(UHFM_sigfox_message_t* sigfox_message, NODE_access_status_t* send_status);
#endif

#endif /* __UHFM_H__ */
