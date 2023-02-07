/*
 * r4s8cr.h
 *
 *  Created on: 2 feb. 2023
 *      Author: Ludo
 */

#ifndef __R4S8CR_H__
#define __R4S8CR_H__

#include "node_common.h"
#include "node_status.h"

/*** R4S8CR macros ***/

#define R4S8CR_TIMEOUT_MS		100
#define R4S8CR_RS485_ADDRESS	0xFF

/*** R4S8CR structures ***/

typedef enum {
	R4S8CR_SUCCESS = 0,
	R4S8CR_ERROR_BASE_LAST = 0x0100
} R4S8CR_status_t;

typedef enum {
	R4S8CR_REGISTER_RELAY_1 = 0,
	R4S8CR_REGISTER_RELAY_2,
	R4S8CR_REGISTER_RELAY_3,
	R4S8CR_REGISTER_RELAY_4,
	R4S8CR_REGISTER_RELAY_5,
	R4S8CR_REGISTER_RELAY_6,
	R4S8CR_REGISTER_RELAY_7,
	R4S8CR_REGISTER_RELAY_8,
	R4S8CR_REGISTER_LAST,
} R4S8CR_register_address_t;

typedef enum {
	R4S8CR_STRING_DATA_INDEX_RELAY_1 = 0,
	R4S8CR_STRING_DATA_INDEX_RELAY_2,
	R4S8CR_STRING_DATA_INDEX_RELAY_3,
	R4S8CR_STRING_DATA_INDEX_RELAY_4,
	R4S8CR_STRING_DATA_INDEX_RELAY_5,
	R4S8CR_STRING_DATA_INDEX_RELAY_6,
	R4S8CR_STRING_DATA_INDEX_RELAY_7,
	R4S8CR_STRING_DATA_INDEX_RELAY_8,
	R4S8CR_STRING_DATA_INDEX_LAST,
} R4S8CR_string_data_index_t;

static const STRING_format_t R4S8CR_REGISTERS_FORMAT[R4S8CR_REGISTER_LAST] = {
	STRING_FORMAT_BOOLEAN,
	STRING_FORMAT_BOOLEAN,
	STRING_FORMAT_BOOLEAN,
	STRING_FORMAT_BOOLEAN,
	STRING_FORMAT_BOOLEAN,
	STRING_FORMAT_BOOLEAN,
	STRING_FORMAT_BOOLEAN,
	STRING_FORMAT_BOOLEAN
};

/*** R4S8CR functions ***/

NODE_status_t R4S8CR_read_register(NODE_read_parameters_t* read_params, NODE_read_data_t* read_data, NODE_access_status_t* read_status);
NODE_status_t R4S8CR_write_register(NODE_write_parameters_t* write_params, NODE_access_status_t* write_status);
NODE_status_t R4S8CR_scan(NODE_t* nodes_list, uint8_t nodes_list_size, uint8_t* nodes_count);
NODE_status_t R4S8CR_update_data(NODE_address_t rs485_address, uint8_t string_data_index, NODE_single_data_ptr_t* single_data_ptr);
NODE_status_t R4S8CR_get_sigfox_payload(NODE_sigfox_payload_type_t payload_type, uint8_t* ul_payload, uint8_t* ul_payload_size);
void R4S8CR_fill_rx_buffer(uint8_t rx_byte);

#endif /* __R4S8CR_H__ */
