/*
 * r4s8cr.h
 *
 *  Created on: 2 feb. 2023
 *      Author: Ludo
 */

#ifndef __R4S8CR_H__
#define __R4S8CR_H__

#include "node.h"

/*** R4S8CR macros ***/

#define R4S8CR_DEFAULT_TIMEOUT_MS	100
#define R4S8CR_NODE_ADDRESS			0xFF

/*** R4S8CR structures ***/

typedef enum {
	R4S8CR_REGISTER_RELAY_1_STATE = 0,
	R4S8CR_REGISTER_RELAY_2_STATE,
	R4S8CR_REGISTER_RELAY_3_STATE,
	R4S8CR_REGISTER_RELAY_4_STATE,
	R4S8CR_REGISTER_RELAY_5_STATE,
	R4S8CR_REGISTER_RELAY_6_STATE,
	R4S8CR_REGISTER_RELAY_7_STATE,
	R4S8CR_REGISTER_RELAY_8_STATE,
	R4S8CR_REGISTER_LAST,
} R4S8CR_register_address_t;

typedef enum {
	R4S8CR_STRING_DATA_INDEX_RELAY_1_STATE = 0,
	R4S8CR_STRING_DATA_INDEX_RELAY_2_STATE,
	R4S8CR_STRING_DATA_INDEX_RELAY_3_STATE,
	R4S8CR_STRING_DATA_INDEX_RELAY_4_STATE,
	R4S8CR_STRING_DATA_INDEX_RELAY_5_STATE,
	R4S8CR_STRING_DATA_INDEX_RELAY_6_STATE,
	R4S8CR_STRING_DATA_INDEX_RELAY_7_STATE,
	R4S8CR_STRING_DATA_INDEX_RELAY_8_STATE,
	R4S8CR_STRING_DATA_INDEX_LAST,
} R4S8CR_string_data_index_t;

static const STRING_format_t R4S8CR_REGISTER_FORMAT[R4S8CR_REGISTER_LAST] = {
	STRING_FORMAT_BOOLEAN,
	STRING_FORMAT_BOOLEAN,
	STRING_FORMAT_BOOLEAN,
	STRING_FORMAT_BOOLEAN,
	STRING_FORMAT_BOOLEAN,
	STRING_FORMAT_BOOLEAN,
	STRING_FORMAT_BOOLEAN,
	STRING_FORMAT_BOOLEAN
};

static const uint32_t R4S8CR_REGISTER_WRITE_TIMEOUT_MS[R4S8CR_REGISTER_LAST] = {
	R4S8CR_DEFAULT_TIMEOUT_MS,
	R4S8CR_DEFAULT_TIMEOUT_MS,
	R4S8CR_DEFAULT_TIMEOUT_MS,
	R4S8CR_DEFAULT_TIMEOUT_MS,
	R4S8CR_DEFAULT_TIMEOUT_MS,
	R4S8CR_DEFAULT_TIMEOUT_MS,
	R4S8CR_DEFAULT_TIMEOUT_MS,
	R4S8CR_DEFAULT_TIMEOUT_MS
};

/*** R4S8CR functions ***/

NODE_status_t R4S8CR_read_register(NODE_read_parameters_t* read_params, NODE_read_data_t* read_data, NODE_access_status_t* read_status);
NODE_status_t R4S8CR_write_register(NODE_write_parameters_t* write_params, NODE_access_status_t* write_status);
NODE_status_t R4S8CR_scan(NODE_t* nodes_list, uint8_t nodes_list_size, uint8_t* nodes_count);
NODE_status_t R4S8CR_update_data(NODE_data_update_t* data_update);
NODE_status_t R4S8CR_get_sigfox_ul_payload(int32_t* integer_data_value, NODE_sigfox_ul_payload_type_t ul_payload_type, uint8_t* ul_payload, uint8_t* ul_payload_size);
void R4S8CR_fill_rx_buffer(uint8_t rx_byte);

#endif /* __R4S8CR_H__ */
