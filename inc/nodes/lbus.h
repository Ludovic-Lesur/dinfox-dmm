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
#include "nvm.h"
#include "parser.h"
#include "string.h"
#include "types.h"

/*** LBUS macros ***/

#define LBUS_TIMEOUT_MS		100

/*** LBUS structures ***/

typedef enum {
	LBUS_SUCCESS = 0,
	LBUS_ERROR_NULL_PARAMETER,
	LBUS_ERROR_NODE_ADDRESS,
	LBUS_ERROR_REPLY_TYPE,
	LBUS_ERROR_BASE_LPUART = 0x0100,
	LBUS_ERROR_BASE_LPTIM = (LBUS_ERROR_BASE_LPUART + LPUART_ERROR_BASE_LAST),
	LBUS_ERROR_BASE_NVM = (LBUS_ERROR_BASE_LPTIM + LPTIM_ERROR_BASE_LAST),
	LBUS_ERROR_BASE_STRING = (LBUS_ERROR_BASE_NVM + NVM_ERROR_BASE_LAST),
	LBUS_ERROR_BASE_PARSER = (LBUS_ERROR_BASE_STRING + STRING_ERROR_BASE_LAST),
	LBUS_ERROR_BASE_LAST = (LBUS_ERROR_BASE_PARSER + PARSER_ERROR_BASE_LAST)
} LBUS_status_t;

/*** LBUS functions ***/

#ifdef AM
void LBUS_init(NODE_address_t self_address);
#else
void LBUS_init(void);
#endif
LBUS_status_t LBUS_read_register(NODE_read_parameters_t* read_params, NODE_reply_t* reply);
LBUS_status_t LBUS_write_register(NODE_write_parameters_t* write_params, NODE_reply_t* reply);
LBUS_status_t LBUS_scan(NODE_t* nodes_list, uint8_t nodes_list_size, uint8_t* nodes_count);
void LBUS_fill_rx_buffer(uint8_t rx_byte);

#define LBUS_status_check(error_base) { if (lbus_status != LBUS_SUCCESS) { status = error_base + lbus_status; goto errors; }}
#define LBUS_error_check() { ERROR_status_check(lbus_status, LBUS_SUCCESS, ERROR_BASE_LBUS); }
#define LBUS_error_check_print() { ERROR_status_check_print(lbus_status, LBUS_SUCCESS, ERROR_BASE_LBUS); }

#endif /* __LBUS_H__ */
