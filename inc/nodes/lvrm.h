/*
 * lvrm.h
 *
 *  Created on: 21 jan. 2023
 *      Author: Ludo
 */

#ifndef __LVRM_H__
#define __LVRM_H__

#include "dinfox.h"
#include "node_common.h"
#include "string.h"
#include "types.h"

/*** LVRM structures ***/

typedef enum {
	LVRM_REGISTER_VCOM_MV = DINFOX_REGISTER_LAST,
	LVRM_REGISTER_VOUT_MV,
	LVRM_REGISTER_IOUT_UA,
	LVRM_REGISTER_OUT_EN,
	LVRM_REGISTER_LAST,
} LVRM_register_address_t;

typedef enum {
	LVRM_STRING_DATA_INDEX_VCOM_MV = DINFOX_STRING_DATA_INDEX_LAST,
	LVRM_STRING_DATA_INDEX_VOUT_MV,
	LVRM_STRING_DATA_INDEX_IOUT_UA,
	LVRM_STRING_DATA_INDEX_OUT_EN,
	LVRM_STRING_DATA_INDEX_LAST,
} LVRM_string_data_index_t;

/*** LVRM macros ***/

#define LVRM_NUMBER_OF_SPECIFIC_REGISTERS	(LVRM_REGISTER_LAST - DINFOX_REGISTER_LAST)
#define LVRM_NUMBER_OF_SPECIFIC_STRING_DATA	(LVRM_STRING_DATA_INDEX_LAST - DINFOX_STRING_DATA_INDEX_LAST)

static const STRING_format_t LVRM_REGISTERS_FORMAT[LVRM_NUMBER_OF_SPECIFIC_REGISTERS] = {
	STRING_FORMAT_DECIMAL,
	STRING_FORMAT_DECIMAL,
	STRING_FORMAT_DECIMAL,
	STRING_FORMAT_BOOLEAN
};

/*** LVRM functions ***/

NODE_status_t LVRM_update_specific_data(NODE_address_t rs485_address, uint8_t string_data_index, NODE_single_data_ptr_t* single_data_ptr);
NODE_status_t LVRM_get_sigfox_payload(NODE_sigfox_payload_type_t sigfox_payload_type, uint8_t* ul_payload, uint8_t* ul_payload_size);

#endif /* __LVRM_H__ */
