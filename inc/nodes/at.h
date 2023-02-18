/*
 * at.h
 *
 *  Created on: 18 feb. 2023
 *      Author: Ludo
 */

#ifndef __AT_H__
#define __AT_H__

#include "node.h"
#include "types.h"

/*** AT macros ***/

#define AT_DEFAULT_TIMEOUT_MS	100

/*** AT functions ***/

NODE_status_t AT_send_command(NODE_command_parameters_t* command_params, NODE_reply_parameters_t* reply_params, NODE_read_data_t* read_data, NODE_access_status_t* command_status);
NODE_status_t AT_read_register(NODE_read_parameters_t* read_params, NODE_read_data_t* read_data, NODE_access_status_t* read_status);
NODE_status_t AT_write_register(NODE_write_parameters_t* write_params, NODE_access_status_t* write_status);
NODE_status_t AT_scan(NODE_t* nodes_list, uint8_t nodes_list_size, uint8_t* nodes_count);
void AT_fill_rx_buffer(uint8_t rx_byte);

#endif /* __AT_H__ */
