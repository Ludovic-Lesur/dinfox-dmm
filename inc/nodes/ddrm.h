/*
 * ddrm.h
 *
 *  Created on: 26 feb. 2023
 *      Author: Ludo
 */

#ifndef __DDRM_H__
#define __DDRM_H__

#include "at_bus.h"
#include "dinfox.h"
#include "node.h"
#include "string.h"
#include "types.h"

/*** DDRM structures ***/

typedef enum {
	DDRM_REGISTER_VIN_MV = DINFOX_REGISTER_LAST,
	DDRM_REGISTER_VOUT_MV,
	DDRM_REGISTER_IOUT_UA,
	DDRM_REGISTER_DC_DC_ENABLE,
	DDRM_REGISTER_LAST,
} DDRM_register_address_t;

typedef enum {
	DDRM_STRING_DATA_INDEX_VCOM_MV = DINFOX_STRING_DATA_INDEX_LAST,
	DDRM_STRING_DATA_INDEX_VOUT_MV,
	DDRM_STRING_DATA_INDEX_IOUT_UA,
	DDRM_STRING_DATA_INDEX_DC_DC_ENABLE,
	DDRM_STRING_DATA_INDEX_LAST,
} DDRM_string_data_index_t;

/*** DDRM macros ***/

#define DDRM_NUMBER_OF_SPECIFIC_REGISTERS	(DDRM_REGISTER_LAST - DINFOX_REGISTER_LAST)
#define DDRM_NUMBER_OF_SPECIFIC_STRING_DATA	(DDRM_STRING_DATA_INDEX_LAST - DINFOX_STRING_DATA_INDEX_LAST)

static const STRING_format_t DDRM_REGISTER_FORMAT[DDRM_NUMBER_OF_SPECIFIC_REGISTERS] = {
	STRING_FORMAT_DECIMAL,
	STRING_FORMAT_DECIMAL,
	STRING_FORMAT_DECIMAL,
	STRING_FORMAT_BOOLEAN
};

static const uint32_t DDRM_REGISTER_WRITE_TIMEOUT_MS[DDRM_NUMBER_OF_SPECIFIC_REGISTERS] = {
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS
};

/*** DDRM functions ***/

NODE_status_t DDRM_update_data(NODE_data_update_t* data_update);
NODE_status_t DDRM_get_sigfox_ul_payload(int32_t* integer_data_value, NODE_sigfox_ul_payload_type_t ul_payload_type, uint8_t* ul_payload, uint8_t* ul_payload_size);

#endif /* __DDRM_H__ */
