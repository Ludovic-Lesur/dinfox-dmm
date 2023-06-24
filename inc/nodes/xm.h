/*
 * xm.h
 *
 *  Created on: 14 jun. 2023
 *      Author: Ludo
 */

#ifndef __XM_H__
#define __XM_H__

#include "node.h"
#include "node_common.h"

/*** XM structures ***/

typedef struct {
	uint32_t* addr_list;
	uint8_t size;
} XM_registers_list_t;

typedef struct {
	uint32_t* value;
	uint32_t* error;
} XM_node_registers_t;

/*** XM functions ***/

NODE_status_t XM_write_line_data(NODE_line_data_write_t* line_data_write, NODE_line_data_t* node_line_data, uint32_t* node_write_timeout, NODE_access_status_t* write_status);
NODE_status_t XM_read_register(NODE_address_t node_addr, uint32_t reg_addr, uint32_t reg_error_value, uint32_t* reg_value, NODE_access_status_t* read_status);

NODE_status_t XM_perform_measurements(NODE_address_t node_addr, NODE_access_status_t* write_status);

NODE_status_t XM_reset_registers(XM_registers_list_t* reg_list, XM_node_registers_t* node_reg);
NODE_status_t XM_read_registers(NODE_address_t node_addr, XM_registers_list_t* reg_list, XM_node_registers_t* node_reg);

#endif /* __XM_H__ */
