/*
 * dmm.h
 *
 *  Created on: 08 jan. 2023
 *      Author: Ludo
 */

#ifndef __DMM_H__
#define __DMM_H__

#include "dinfox.h"

typedef enum {
	DMM_REGISTER_VUSB_MV = DINFOX_REGISTER_LAST,
	DMM_REGISTER_VRS_MV,
	DMM_REGISTER_VHMI_MV,
	DMM_REGISTER_NODES_COUNT,
	DMM_REGISTER_SIGFOX_UL_PERIOD_SECONDS,
	DMM_REGISTER_SIGFOX_DL_PERIOD_SECONDS,
	DMM_REGISTER_LAST,
} DMM_register_address_t;

typedef enum {
	DMM_STRING_DATA_INDEX_VUSB_MV = DINFOX_STRING_DATA_INDEX_LAST,
	DMM_STRING_DATA_INDEX_VRS_MV,
	DMM_STRING_DATA_INDEX_VHMI_MV,
	DMM_STRING_DATA_INDEX_NODES_COUNT,
	DMM_STRING_DATA_INDEX_SIGFOX_UL_PERIOD_SECONDS,
	DMM_STRING_DATA_INDEX_SIGFOX_DL_PERIOD_SECONDS,
	DMM_STRING_DATA_INDEX_LAST,
} DMM_string_data_index_t;

/*** DMM macros ***/

#define DMM_NUMBER_OF_SPECIFIC_REGISTERS	(DMM_REGISTER_LAST - DINFOX_REGISTER_LAST)
#define DMM_NUMBER_OF_SPECIFIC_STRING_DATA	(DMM_STRING_DATA_INDEX_LAST - DINFOX_STRING_DATA_INDEX_LAST)

static const STRING_format_t DMM_REGISTERS_FORMAT[DMM_NUMBER_OF_SPECIFIC_REGISTERS] = {
	STRING_FORMAT_DECIMAL,
	STRING_FORMAT_DECIMAL,
	STRING_FORMAT_DECIMAL,
	STRING_FORMAT_DECIMAL,
	STRING_FORMAT_DECIMAL,
	STRING_FORMAT_DECIMAL,
};

/*** DMM functions ***/

NODE_status_t DMM_read_register(NODE_read_parameters_t* read_params, NODE_read_data_t* read_data, NODE_access_status_t* read_status);
NODE_status_t DMM_write_register(NODE_write_parameters_t* write_params, NODE_access_status_t* write_status);
NODE_status_t DMM_update_data(NODE_data_update_t* data_update);
NODE_status_t DMM_get_sigfox_ul_payload(int32_t* integer_data_value, NODE_sigfox_ul_payload_type_t ul_payload_type, uint8_t* ul_payload, uint8_t* ul_payload_size);

#endif /* __DMM_H__ */
