/*
 * node.h
 *
 *  Created on: 22 jan 2023
 *      Author: Ludo
 */

#ifndef __NODE_H__
#define __NODE_H__

#include "lptim.h"
#include "lpuart.h"
#include "string.h"
#include "types.h"

/*** NODES macros ***/

#define NODES_LIST_SIZE_MAX				32
#define NODE_STRING_BUFFER_SIZE			32

#define NODE_ERROR_STRING				"ERROR"
#define NODE_ERROR_VALUE_NODE_ADDRESS	0xFF
#define NODE_ERROR_VALUE_BOARD_ID		0xFF
#define NODE_ERROR_VALUE_VERSION		0xFF
#define NODE_ERROR_VALUE_COMMIT_INDEX	0xFF
#define NODE_ERROR_VALUE_COMMIT_ID		0x0FFFFFFF
#define NODE_ERROR_VALUE_DIRTY_FLAG		0x0F
#define NODE_ERROR_VALUE_RESET_REASON	0xFF
#define NODE_ERROR_VALUE_ERROR_STACK	0xFFFF
#define NODE_ERROR_VALUE_ANALOG_16BITS	0xFFFF
#define NODE_ERROR_VALUE_ANALOG_23BITS	0x7FFFFF
#define NODE_ERROR_VALUE_TEMPERATURE	0x7F
#define NODE_ERROR_VALUE_HUMIDITY		0xFF
#define NODE_ERROR_VALUE_BOOLEAN		0

/*** NODE structures ***/

typedef enum {
	NODE_SUCCESS = 0,
	NODE_ERROR_NOT_SUPPORTED,
	NODE_ERROR_NULL_PARAMETER,
	NODE_ERROR_PROTOCOL,
	NODE_ERROR_NODE_ADDRESS,
	NODE_ERROR_REGISTER_ADDRESS,
	NODE_ERROR_REGISTER_FORMAT,
	NODE_ERROR_STRING_DATA_INDEX,
	NODE_ERROR_READ_TYPE,
	NODE_ERROR_ACCESS,
	NODE_ERROR_NONE_RADIO_MODULE,
	NODE_ERROR_SIGFOX_PAYLOAD_TYPE,
	NODE_ERROR_SIGFOX_PAYLOAD_EMPTY,
	NODE_ERROR_SIGFOX_LOOP,
	NODE_ERROR_SIGFOX_SEND,
	NODE_ERROR_DOWNLINK_NODE_ADDRESS,
	NODE_ERROR_DOWNLINK_BOARD_ID,
	NODE_ERROR_DOWNLINK_OPERATION_CODE,
	NODE_ERROR_ACTION_INDEX,
	NODE_ERROR_BASE_LPUART = 0x0100,
	NODE_ERROR_BASE_LPTIM = (NODE_ERROR_BASE_LPUART + LPUART_ERROR_BASE_LAST),
	NODE_ERROR_BASE_STRING = (NODE_ERROR_BASE_LPTIM + LPTIM_ERROR_BASE_LAST),
	NODE_ERROR_BASE_LAST = (NODE_ERROR_BASE_STRING + STRING_ERROR_BASE_LAST)
} NODE_status_t;

#ifdef AM
typedef uint8_t	NODE_address_t;
#endif

typedef struct {
#ifdef AM
	NODE_address_t address;
#endif
	uint8_t board_id;
} NODE_t;

typedef struct {
	NODE_t list[NODES_LIST_SIZE_MAX];
	uint8_t count;
} NODE_list_t;

typedef enum {
	NODE_REPLY_TYPE_NONE = 0,
	NODE_REPLY_TYPE_RAW,
	NODE_REPLY_TYPE_OK,
	NODE_REPLY_TYPE_VALUE,
	NODE_REPLY_TYPE_BYTE_ARRAY,
	NODE_REPLY_TYPE_LAST
} NODE_reply_type_t;

typedef struct {
#ifdef AM
	NODE_address_t node_address;
#endif
	char_t* command;
} NODE_command_parameters_t;

typedef struct {
	NODE_reply_type_t type;
	STRING_format_t format; // Expected value format.
	uint32_t timeout_ms;
	// For byte array.
	uint8_t byte_array_size;
	uint8_t exact_length;
} NODE_reply_parameters_t;

typedef struct {
#ifdef AM
	NODE_address_t node_address;
#endif
	uint8_t register_address;
	uint32_t timeout_ms;
	STRING_format_t format; // Expected value format.
	NODE_reply_type_t type;
} NODE_read_parameters_t;

typedef struct {
	char_t* raw;
	int32_t value;
	uint8_t* byte_array;
	uint8_t extracted_length;
} NODE_read_data_t;

typedef struct {
#ifdef AM
	NODE_address_t node_address;
#endif
	uint8_t register_address;
	uint32_t timeout_ms;
	STRING_format_t format; // Register value format.
	int32_t value;
} NODE_write_parameters_t;

typedef union {
	struct {
		unsigned error_received : 1;
		unsigned parser_error : 1;
		unsigned reply_timeout : 1;
		unsigned sequence_timeout : 1;
	};
	uint8_t all;
} NODE_access_status_t;

typedef struct {
#ifdef AM
	NODE_address_t node_address;
#endif
	uint8_t string_data_index;
	char_t* name_ptr;
	char_t* value_ptr;
	int32_t* registers_value_ptr;
} NODE_data_update_t;

typedef enum {
	NODE_SIGFOX_PAYLOAD_TYPE_STARTUP = 0,
	NODE_SIGFOX_PAYLOAD_TYPE_MONITORING,
	NODE_SIGFOX_PAYLOAD_TYPE_DATA,
	NODE_SIGFOX_PAYLOAD_TYPE_LAST
} NODE_sigfox_ul_payload_type_t;

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

NODE_status_t NODE_task(void);

#define NODE_append_string_name(str) { \
	string_status = STRING_append_string((data_update -> name_ptr), NODE_STRING_BUFFER_SIZE, str, &buffer_size); \
	STRING_status_check(NODE_ERROR_BASE_STRING); \
}

#define NODE_append_string_value(str) { \
	string_status = STRING_append_string((data_update -> value_ptr), NODE_STRING_BUFFER_SIZE, str, &buffer_size); \
	STRING_status_check(NODE_ERROR_BASE_STRING); \
}

#define NODE_flush_string_value(void) { \
	uint8_t char_idx = 0; \
	for (char_idx=0 ; char_idx<NODE_STRING_BUFFER_SIZE ; char_idx++) { \
		(data_update -> value_ptr)[char_idx] = STRING_CHAR_NULL; \
	} \
	buffer_size = 0; \
}

#define NODE_update_value(addr, val) { \
	(data_update -> registers_value_ptr)[addr] = val; \
}

#define NODE_status_check(error_base) { if (node_status != NODE_SUCCESS) { status = error_base + node_status; goto errors; }}
#define NODE_error_check() { ERROR_status_check(node_status, NODE_SUCCESS, ERROR_BASE_NODE); }
#define NODE_error_check_print() { ERROR_status_check_print(node_status, NODE_SUCCESS, ERROR_BASE_NODE); }

#endif /* __NODE_H__ */
