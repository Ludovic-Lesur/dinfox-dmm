/*
 * rs485.h
 *
 *  Created on: 28 oct. 2022
 *      Author: Ludo
 */

#ifndef __RS485_H__
#define __RS485_H__

#include "lptim.h"
#include "lpuart.h"
#include "mode.h"
#include "nvm.h"
#include "parser.h"
#include "rs485_common.h"
#include "string.h"

/*** RS485 structures ***/

typedef enum {
	RS485_SUCCESS,
	RS485_ERROR_NULL_PARAMETER,
	RS485_ERROR_NULL_SIZE,
	RS485_ERROR_MODE,
	RS485_ERROR_REPLY_TYPE,
	RS485_ERROR_BUFFER_OVERFLOW,
	RS485_ERROR_BASE_LPUART = 0x0100,
	RS485_ERROR_BASE_LPTIM = (RS485_ERROR_BASE_LPUART + LPUART_ERROR_BASE_LAST),
	RS485_ERROR_BASE_NVM = (RS485_ERROR_BASE_LPTIM + LPTIM_ERROR_BASE_LAST),
	RS485_ERROR_BASE_STRING = (RS485_ERROR_BASE_NVM + NVM_ERROR_BASE_LAST),
	RS485_ERROR_BASE_PARSER = (RS485_ERROR_BASE_STRING + STRING_ERROR_BASE_LAST),
	RS485_ERROR_BASE_LAST = (RS485_ERROR_BASE_PARSER + PARSER_ERROR_BASE_LAST)
} RS485_status_t;

typedef enum {
	RS485_REPLY_TYPE_RAW = 0,
	RS485_REPLY_TYPE_OK,
	RS485_REPLY_TYPE_VALUE,
	RS485_REPLY_TYPE_LAST
} RS485_reply_type_t;

typedef union {
	struct {
		unsigned error_received : 1;
		unsigned reply_timeout : 1;
		unsigned parser_error : 1;
		unsigned sequence_timeout : 1;
		unsigned source_address_mismatch : 1;
	};
	uint8_t all;
} RS485_reply_status_t;

typedef struct {
#ifdef AM
	RS485_address_t node_address;
#endif
	uint8_t register_address;
	uint32_t timeout_ms;
	STRING_format_t format; // Expected value format.
	RS485_reply_type_t type;
} RS485_read_input_t;

typedef struct {
#ifdef AM
	RS485_address_t node_address;
#endif
	uint8_t register_address;
	uint32_t timeout_ms;
	STRING_format_t format; // Register value format.
	int32_t value;
} RS485_write_input_t;

typedef struct {
	char_t* raw;
	int32_t value; // For value type.
	RS485_reply_status_t status;
} RS485_reply_t;

/*** RS485 functions ***/

void RS485_init(void);
RS485_status_t RS485_scan_nodes(void);
RS485_status_t RS485_read_register(RS485_read_input_t* read_input_ptr, RS485_reply_t* reply_ptr);
RS485_status_t RS485_write_register(RS485_write_input_t* write_input_ptr, RS485_reply_t* reply_ptr);
void RS485_fill_rx_buffer(uint8_t rx_byte);

#define RS485_status_check(error_base) { if (rs485_status != RS485_SUCCESS) { status = error_base + rs485_status; goto errors; }}
#define RS485_error_check() { ERROR_status_check(rs485_status, RS485_SUCCESS, ERROR_BASE_RS485); }
#define RS485_error_check_print() { ERROR_status_check_print(rs485_status, RS485_SUCCESS, ERROR_BASE_RS485); }

#endif /* __RS485_H__ */
