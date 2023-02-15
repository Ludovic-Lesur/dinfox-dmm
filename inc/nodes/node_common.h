/*
 * node_common.h
 *
 *  Created on: 23 jan. 2023
 *      Author: Ludo
 */

#ifndef __NODE_COMMON_H__
#define __NODE_COMMON_H__

#include "string.h"
#include "types.h"

/*** NODE common macros ***/

#define NODES_LIST_SIZE_MAX				32

#define NODE_STRING_BUFFER_SIZE			32

#define NODE_NODE_TIMEOUT_MS			100

#define NODE_ERROR_STRING				"ERROR"
#define NODE_ERROR_VALUE_RS485_ADDRESS	0xFF
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
#define NODE_ERROR_VALUE_RELAY_STATE	0

/*** NODES common structures ***/

typedef uint8_t	NODE_address_t;

typedef struct {
	NODE_address_t address;
	uint8_t board_id;
} NODE_t;

typedef enum {
	NODE_REPLY_TYPE_NONE = 0,
	NODE_REPLY_TYPE_RAW,
	NODE_REPLY_TYPE_OK,
	NODE_REPLY_TYPE_VALUE,
	NODE_REPLY_TYPE_LAST
} NODE_reply_type_t;

typedef union {
	struct {
		unsigned error_received : 1;
		unsigned reply_timeout : 1;
		unsigned parser_error : 1;
		unsigned sequence_timeout : 1;
		unsigned source_address_mismatch : 1;
	};
	uint8_t all;
} NODE_access_status_t;

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

typedef struct {
	char_t* name_ptr;
	char_t* value_ptr;
} NODE_single_string_data_t;

typedef enum {
	NODE_SIGFOX_PAYLOAD_TYPE_STARTUP = 0,
	NODE_SIGFOX_PAYLOAD_TYPE_MONITORING,
	NODE_SIGFOX_PAYLOAD_TYPE_DATA,
	NODE_SIGFOX_PAYLOAD_TYPE_LAST
} NODE_sigfox_payload_type_t;

/*** NODE common functions ***/

/* APPEND A STRING TO THE NODE DATA NAME LINE.
 * @param str:	String to append
 * @return:		None.
 */
#define NODE_append_string_name(str) { \
	string_status = STRING_append_string((single_string_data -> name_ptr), NODE_STRING_BUFFER_SIZE, str, &buffer_size); \
	STRING_status_check(NODE_ERROR_BASE_STRING); \
}

/* APPEND A STRING TO THE NODE DATA VALUE LINE.
 * @param str:	String to append
 * @return:		None.
 */
#define NODE_append_string_value(str) { \
	string_status = STRING_append_string((single_string_data -> value_ptr), NODE_STRING_BUFFER_SIZE, str, &buffer_size); \
	STRING_status_check(NODE_ERROR_BASE_STRING); \
}

/* FLUSH THE NODE DATA VALUE LINE.
 * @param:	None.
 * @return:	None.
 */
#define NODE_flush_string_value(void) { \
	uint8_t char_idx = 0; \
	for (char_idx=0 ; char_idx<NODE_STRING_BUFFER_SIZE ; char_idx++) { \
		(single_string_data -> value_ptr)[char_idx] = STRING_CHAR_NULL; \
	} \
	buffer_size = 0; \
}

/* UPDATE NODE DATA VALUE.
 * @param val:	New value.
 * @return:		None.
 */
#define NODE_update_value(addr, val) { \
	registers_value[addr] = val; \
}

#endif /* __NODE_COMMON_H__ */
