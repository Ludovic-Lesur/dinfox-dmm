/*
 * node.h
 *
 *  Created on: 22 jan 2023
 *      Author: Ludo
 */

#ifndef __NODE_H__
#define __NODE_H__

#include "node_common.h"
#include "rs485_common.h"
#include "types.h"

/*** NODE functions ***/

NODE_status_t NODE_get_name(RS485_node_t* rs485_node, char_t** board_name);
NODE_status_t NODE_perform_measurements(RS485_node_t* rs485_node);
NODE_status_t NODE_unstack_string_data(RS485_node_t* rs485_node, char_t** measurements_name_ptr, char_t** measurements_value_ptr);
NODE_status_t NODE_get_sigfox_payload(RS485_node_t* rs485_node, uint8_t* ul_payload, uint8_t* ul_payload_size);
NODE_status_t NODE_write(RS485_node_t* rs485_node, uint8_t register_address, uint8_t value);

#define NODE_status_check(error_base) { if (node_status != NODE_SUCCESS) { status = error_base + node_status; goto errors; }}
#define NODE_error_check() { ERROR_status_check(node_status, NODE_SUCCESS, ERROR_BASE_NODE); }
#define NODE_error_check_print() { ERROR_status_check_print(node_status, NODE_SUCCESS, ERROR_BASE_NODE); }

#endif /* __NODE_H__ */
