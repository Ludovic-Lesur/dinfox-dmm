/*
 * at_bus.h
 *
 *  Created on: 18 feb. 2023
 *      Author: Ludo
 */

#ifndef __AT_BUS_H__
#define __AT_BUS_H__

#include "node.h"
#include "types.h"

/*** AT BUS macros ***/

#define AT_BUS_DEFAULT_TIMEOUT_MS	100

/*** AT BUS functions ***/

void AT_BUS_init(void);
void AT_BUS_de_init(void);

NODE_status_t AT_BUS_send_command(NODE_command_parameters_t* command_params);

NODE_status_t AT_BUS_write_register(NODE_access_parameters_t* write_params, uint32_t reg_value, uint32_t reg_mask, NODE_access_status_t* write_status);
NODE_status_t AT_BUS_read_register(NODE_access_parameters_t* read_params, uint32_t* reg_value, NODE_access_status_t* read_status);

NODE_status_t AT_BUS_scan(NODE_t* nodes_list, uint8_t nodes_list_size, uint8_t* nodes_count);

void AT_BUS_fill_rx_buffer(uint8_t rx_byte);

#endif /* __AT_BUS_H__ */
