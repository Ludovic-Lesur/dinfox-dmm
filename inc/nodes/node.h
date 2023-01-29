/*
 * node.h
 *
 *  Created on: 22 jan 2023
 *      Author: Ludo
 */

#ifndef __NODE_H__
#define __NODE_H__

#include "node_common.h"
#include "rs485.h"
#include "rs485_common.h"
#include "types.h"

/*** NODE functions ***/

NODE_status_t NODE_get_name(RS485_node_t* node, char_t** board_name);
NODE_status_t NODE_update_data(RS485_node_t* node, uint8_t string_data_index);
NODE_status_t NODE_update_all_data(RS485_node_t* node);
NODE_status_t NODE_read_string_data(RS485_node_t* node, uint8_t string_data_index, char_t** string_data_name_ptr, char_t** string_data_value_ptr);
NODE_status_t NODE_write_data(RS485_node_t* node, uint8_t string_data_index, uint8_t value, RS485_reply_status_t* write_status);
NODE_status_t NODE_get_sigfox_payload(RS485_node_t* node, uint8_t* ul_payload, uint8_t* ul_payload_size);

#define NODE_status_check(error_base) { if (node_status != NODE_SUCCESS) { status = error_base + node_status; goto errors; }}
#define NODE_error_check() { ERROR_status_check(node_status, NODE_SUCCESS, ERROR_BASE_NODE); }
#define NODE_error_check_print() { ERROR_status_check_print(node_status, NODE_SUCCESS, ERROR_BASE_NODE); }

#endif /* __NODE_H__ */
