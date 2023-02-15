/*
 * lbus.h
 *
 *  Created on: 28 oct. 2022
 *      Author: Ludo
 */

#ifndef __LBUS_H__
#define __LBUS_H__

#include "lptim.h"
#include "lpuart.h"
#include "mode.h"
#include "node_common.h"
#include "node_status.h"
#include "nvm.h"
#include "parser.h"
#include "string.h"
#include "types.h"

/*** LBUS macros ***/

#define LBUS_TIMEOUT_MS		100

/*** LBUS functions ***/

NODE_status_t LBUS_send_command(NODE_command_parameters_t* command_params, NODE_reply_parameters_t* reply_params, NODE_read_data_t* read_data, NODE_access_status_t* command_status);
NODE_status_t LBUS_read_register(NODE_read_parameters_t* read_params, NODE_read_data_t* read_data, NODE_access_status_t* read_status);
NODE_status_t LBUS_write_register(NODE_write_parameters_t* write_params, NODE_access_status_t* write_status);
NODE_status_t LBUS_scan(NODE_t* nodes_list, uint8_t nodes_list_size, uint8_t* nodes_count);
void LBUS_fill_rx_buffer(uint8_t rx_byte);

#define LBUS_status_check(error_base) { if (lbus_status != LBUS_SUCCESS) { status = error_base + lbus_status; goto errors; }}
#define LBUS_error_check() { ERROR_status_check(lbus_status, LBUS_SUCCESS, ERROR_BASE_LBUS); }
#define LBUS_error_check_print() { ERROR_status_check_print(lbus_status, LBUS_SUCCESS, ERROR_BASE_LBUS); }

#endif /* __LBUS_H__ */
