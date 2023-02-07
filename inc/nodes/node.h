/*
 * node.h
 *
 *  Created on: 22 jan 2023
 *      Author: Ludo
 */

#ifndef __NODE_H__
#define __NODE_H__

#include "node_common.h"
#include "node_status.h"
#include "types.h"

/*** NODE structures ***/

typedef struct {
	NODE_t list[NODES_LIST_SIZE_MAX];
	uint8_t count;
} NODE_list_t;

/*** NODES global variables ***/

NODE_list_t NODES_LIST;

/*** NODE functions ***/

void NODE_init(void);
NODE_status_t NODE_scan(void);

NODE_status_t NODE_update_data(NODE_t* node, uint8_t string_data_index);
NODE_status_t NODE_update_all_data(NODE_t* node);

NODE_status_t NODE_get_name(NODE_t* node, char_t** board_name);
NODE_status_t NODE_get_last_string_data_index(NODE_t* node, uint8_t* last_string_data_index);
NODE_status_t NODE_read_string_data(NODE_t* node, uint8_t string_data_index, char_t** string_data_name_ptr, char_t** string_data_value_ptr);
NODE_status_t NODE_write_string_data(NODE_t* node, uint8_t string_data_index, int32_t value, NODE_access_status_t* write_status);

NODE_status_t NODE_get_last_register_address(NODE_t* node, uint8_t* last_register_address);
NODE_status_t NODE_read_register(NODE_t* node, uint8_t register_address, int32_t* value, NODE_access_status_t* read_status);
NODE_status_t NODE_write_register(NODE_t* node, uint8_t register_address, int32_t value, NODE_access_status_t* write_status);

NODE_status_t NODE_get_sigfox_payload(NODE_t* node, NODE_sigfox_payload_type_t sigfox_payload_type, uint8_t* ul_payload, uint8_t* ul_payload_size);

#define NODE_status_check(error_base) { if (node_status != NODE_SUCCESS) { status = error_base + node_status; goto errors; }}
#define NODE_error_check() { ERROR_status_check(node_status, NODE_SUCCESS, ERROR_BASE_NODE); }
#define NODE_error_check_print() { ERROR_status_check_print(node_status, NODE_SUCCESS, ERROR_BASE_NODE); }

#endif /* __NODE_H__ */
