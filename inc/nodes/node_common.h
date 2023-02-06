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

#define NODES_LIST_SIZE_MAX			32

#define NODE_STRING_BUFFER_SIZE		16
#define NODE_STRING_DATA_ERROR		"ERROR"

#define NODE_NODE_TIMEOUT_MS		100

/*** NODES common structures ***/

typedef uint8_t	NODE_address_t;

typedef struct {
	NODE_address_t address;
	uint8_t board_id;
} NODE_t;

typedef enum {
	NODE_READ_TYPE_NONE = 0,
	NODE_READ_TYPE_RAW,
	NODE_READ_TYPE_OK,
	NODE_READ_TYPE_VALUE,
	NODE_READ_TYPE_LAST
} NODE_read_type_t;

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
	uint8_t register_address;
	uint32_t timeout_ms;
	STRING_format_t format; // Expected value format.
	NODE_read_type_t type;
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
	char_t* string_name_ptr;
	char_t* string_value_ptr;
	int32_t* value_ptr;
} NODE_single_data_ptr_t;

typedef enum {
	NODE_SIGFOX_PAYLOAD_TYPE_STARTUP = 0,
	NODE_SIGFOX_PAYLOAD_TYPE_DATA,
	NODE_SIGFOX_PAYLOAD_TYPE_MONITORING,
	NODE_SIGFOX_PAYLOAD_TYPE_LAST
} NODE_sigfox_payload_type_t;

/*** NODE common functions ***/

/* APPEND A STRING TO THE NODE DATA NAME LINE.
 * @param str:	String to append
 * @return:		None.
 */
#define NODE_append_string_name(str) { \
	string_status = STRING_append_string((single_data_ptr -> string_name_ptr), NODE_STRING_BUFFER_SIZE, str, &buffer_size); \
	STRING_status_check(NODE_ERROR_BASE_STRING); \
}

/* APPEND A STRING TO THE NODE DATA VALUE LINE.
 * @param str:	String to append
 * @return:		None.
 */
#define NODE_append_string_value(str) { \
	string_status = STRING_append_string((single_data_ptr -> string_value_ptr), NODE_STRING_BUFFER_SIZE, str, &buffer_size); \
	STRING_status_check(NODE_ERROR_BASE_STRING); \
}

/* UPDATE NODE DATA VALUE.
 * @param val:	New value.
 * @return:		None.
 */
#define NODE_update_value(val) { \
	(*(single_data_ptr -> value_ptr)) = val; \
}

#endif /* __NODE_COMMON_H__ */
