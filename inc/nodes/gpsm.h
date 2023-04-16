/*
 * gpsm.h
 *
 *  Created on: 16 apr. 2023
 *      Author: Ludo
 */

#ifndef __GPSM_H__
#define __GPSM_H__

#include "at_bus.h"
#include "dinfox.h"
#include "node.h"
#include "string.h"
#include "types.h"

/*** GPSM structures ***/

typedef enum {
	GPSM_REGISTER_VGPS_MV = DINFOX_REGISTER_LAST,
	GPSM_REGISTER_VANT_MV,
	GPSM_REGISTER_LAST,
} GPSM_register_address_t;

typedef enum {
	GPSM_STRING_DATA_INDEX_VGPS_MV = DINFOX_STRING_DATA_INDEX_LAST,
	GPSM_STRING_DATA_INDEX_VANT_MV,
	GPSM_STRING_DATA_INDEX_LAST,
} GPSM_string_data_index_t;

/*** GPSM macros ***/

#define GPSM_NUMBER_OF_SPECIFIC_REGISTERS	(GPSM_REGISTER_LAST - DINFOX_REGISTER_LAST)
#define GPSM_NUMBER_OF_SPECIFIC_STRING_DATA	(GPSM_STRING_DATA_INDEX_LAST - DINFOX_STRING_DATA_INDEX_LAST)

static const STRING_format_t GPSM_REGISTER_FORMAT[GPSM_NUMBER_OF_SPECIFIC_REGISTERS] = {
	STRING_FORMAT_DECIMAL,
	STRING_FORMAT_DECIMAL
};

static const uint32_t GPSM_REGISTER_WRITE_TIMEOUT_MS[GPSM_NUMBER_OF_SPECIFIC_REGISTERS] = {
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS
};

/*** GPSM functions ***/

NODE_status_t GPSM_update_data(NODE_data_update_t* data_update);
NODE_status_t GPSM_get_sigfox_ul_payload(int32_t* integer_data_value, NODE_sigfox_ul_payload_type_t ul_payload_type, uint8_t* ul_payload, uint8_t* ul_payload_size);

#endif /* __GPSM_H__ */
