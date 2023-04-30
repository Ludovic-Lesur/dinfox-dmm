/*
 * lvrm.h
 *
 *  Created on: 21 jan. 2023
 *      Author: Ludo
 */

#ifndef __LVRM_H__
#define __LVRM_H__

#include "at_bus.h"
#include "dinfox.h"
#include "node.h"
#include "string.h"
#include "types.h"

/*** LVRM macros ***/

#define LVRM_RELAY_WRITE_TIMEOUT_MS		2000

/*** LVRM structures ***/

typedef enum {
	LVRM_REGISTER_VCOM_MV = DINFOX_REGISTER_LAST,
	LVRM_REGISTER_VOUT_MV,
	LVRM_REGISTER_IOUT_UA,
	LVRM_REGISTER_RELAY_STATE,
	LVRM_REGISTER_LAST,
} LVRM_register_address_t;

typedef enum {
	LVRM_STRING_DATA_INDEX_VCOM_MV = DINFOX_STRING_DATA_INDEX_LAST,
	LVRM_STRING_DATA_INDEX_VOUT_MV,
	LVRM_STRING_DATA_INDEX_IOUT_UA,
	LVRM_STRING_DATA_INDEX_RELAY_STATE,
	LVRM_STRING_DATA_INDEX_LAST,
} LVRM_string_data_index_t;

/*** LVRM macros ***/

#define LVRM_NUMBER_OF_SPECIFIC_REGISTERS	(LVRM_REGISTER_LAST - DINFOX_REGISTER_LAST)
#define LVRM_NUMBER_OF_SPECIFIC_STRING_DATA	(LVRM_STRING_DATA_INDEX_LAST - DINFOX_STRING_DATA_INDEX_LAST)

static const STRING_format_t LVRM_REGISTER_FORMAT[LVRM_NUMBER_OF_SPECIFIC_REGISTERS] = {
	STRING_FORMAT_DECIMAL,
	STRING_FORMAT_DECIMAL,
	STRING_FORMAT_DECIMAL,
	STRING_FORMAT_BOOLEAN
};

static const uint32_t LVRM_REGISTER_WRITE_TIMEOUT_MS[LVRM_NUMBER_OF_SPECIFIC_REGISTERS] = {
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	LVRM_RELAY_WRITE_TIMEOUT_MS
};

/*** LVRM functions ***/

NODE_status_t LVRM_update_data(NODE_data_update_t* data_update);
NODE_status_t LVRM_get_sigfox_ul_payload(int32_t* integer_data_value, NODE_sigfox_ul_payload_type_t ul_payload_type, uint8_t* ul_payload, uint8_t* ul_payload_size);

#endif /* __LVRM_H__ */
