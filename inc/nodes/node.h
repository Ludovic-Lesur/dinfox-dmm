/*
 * node.h
 *
 *  Created on: 22 jan 2023
 *      Author: Ludo
 */

#ifndef __NODE_H__
#define __NODE_H__

#include "adc.h"
#include "i2c.h"
#include "lptim.h"
#include "lpuart.h"
#include "node_common.h"
#include "string.h"
#include "types.h"

/*** NODES macros ***/

#define NODES_LIST_SIZE_MAX					32
#define NODE_STRING_BUFFER_SIZE				32

static const char_t NODE_ERROR_STRING[] =	"ERROR";
#define NODE_ERROR_VALUE_NODE_ADDRESS		0xFF
#define NODE_ERROR_VALUE_BOARD_ID			0xFF
#define NODE_ERROR_VALUE_VERSION			0xFF
#define NODE_ERROR_VALUE_COMMIT_INDEX		0xFF
#define NODE_ERROR_VALUE_COMMIT_ID			0x0FFFFFFF
#define NODE_ERROR_VALUE_DIRTY_FLAG			0x0F
#define NODE_ERROR_VALUE_RESET_REASON		0xFF
#define NODE_ERROR_VALUE_ERROR_STACK		0xFFFF
#define NODE_ERROR_VALUE_ANALOG_16BITS		0xFFFF
#define NODE_ERROR_VALUE_ANALOG_23BITS		0x7FFFFF
#define NODE_ERROR_VALUE_TEMPERATURE		0x7F
#define NODE_ERROR_VALUE_HUMIDITY			0xFF
#define NODE_ERROR_VALUE_BOOLEAN			0

/*** NODE structures ***/

typedef enum {
	NODE_SUCCESS = 0,
	NODE_ERROR_NOT_SUPPORTED,
	NODE_ERROR_NULL_PARAMETER,
	NODE_ERROR_PROTOCOL,
	NODE_ERROR_NODE_ADDRESS,
	NODE_ERROR_REGISTER_ADDRESS,
	NODE_ERROR_REGISTER_FORMAT,
	NODE_ERROR_REGISTER_READ_ONLY,
	NODE_ERROR_REGISTER_LIST_SIZE,
	NODE_ERROR_LINE_DATA_INDEX,
	NODE_ERROR_REPLY_TYPE,
	NODE_ERROR_ACCESS,
	NODE_ERROR_NONE_RADIO_MODULE,
	NODE_ERROR_SIGFOX_PAYLOAD_TYPE,
	NODE_ERROR_SIGFOX_PAYLOAD_LOOP,
	NODE_ERROR_SIGFOX_SEND,
	NODE_ERROR_SIGFOX_UPLINK_PERIOD,
	NODE_ERROR_SIGFOX_DOWNLINK_PERIOD,
	NODE_ERROR_DOWNLINK_NODE_ADDRESS,
	NODE_ERROR_DOWNLINK_OPERATION_CODE,
	NODE_ERROR_ACTION_INDEX,
	NODE_ERROR_RELAY_ID,
	NODE_ERROR_BASE_ADC = 0x0100,
	NODE_ERROR_BASE_I2C = (NODE_ERROR_BASE_ADC + ADC_ERROR_BASE_LAST),
	NODE_ERROR_BASE_LPUART = (NODE_ERROR_BASE_I2C + I2C_ERROR_BASE_LAST),
	NODE_ERROR_BASE_LPTIM = (NODE_ERROR_BASE_LPUART + LPUART_ERROR_BASE_LAST),
	NODE_ERROR_BASE_STRING = (NODE_ERROR_BASE_LPTIM + LPTIM_ERROR_BASE_LAST),
	NODE_ERROR_BASE_LAST = (NODE_ERROR_BASE_STRING + STRING_ERROR_BASE_LAST)
} NODE_status_t;

typedef struct {
	NODE_address_t address;
	uint8_t board_id;
	uint8_t startup_data_sent;
	uint8_t radio_transmission_count;
} NODE_t;

typedef struct {
	NODE_t list[NODES_LIST_SIZE_MAX];
	uint8_t count;
} NODE_list_t;

typedef struct {
	char_t* name;
	char_t* unit;
	STRING_format_t print_format;
	uint8_t print_prefix;
	uint8_t reg_addr;
	uint32_t field_mask;
} NODE_line_data_t;

typedef struct {
	NODE_address_t node_addr;
	uint8_t line_data_index;
	char_t* name_ptr;
	char_t* value_ptr;
} NODE_line_data_read_t;

typedef struct {
	NODE_address_t node_addr;
	uint8_t line_data_index;
	uint32_t field_value;
} NODE_line_data_write_t;

typedef struct {
	NODE_t* node;
	uint8_t* ul_payload;
	uint8_t* size;
} NODE_ul_payload_update_t;

/*** NODES global variables ***/

NODE_list_t NODES_LIST;

/*** NODE functions ***/

void NODE_init(void);
NODE_status_t NODE_scan(void);

NODE_status_t NODE_write_line_data(NODE_t* node, uint8_t line_data_index, uint32_t value, NODE_access_status_t* write_status);
NODE_status_t NODE_read_line_data(NODE_t* node, uint8_t line_data_index, NODE_access_status_t* read_status);
NODE_status_t NODE_read_line_data_all(NODE_t* node);

NODE_status_t NODE_get_name(NODE_t* node, char_t** board_name);
NODE_status_t NODE_get_last_line_data_index(NODE_t* node, uint8_t* last_line_data_index);
NODE_status_t NODE_get_line_data(NODE_t* node, uint8_t line_data_index, char_t** line_data_name_ptr, char_t** line_data_value_ptr);

void NODE_task(void);

#define NODE_append_name_string(str) { \
	string_status = STRING_append_string((line_data_read -> name_ptr), NODE_STRING_BUFFER_SIZE, str, &buffer_size); \
	STRING_check_status(NODE_ERROR_BASE_STRING); \
}

#define NODE_append_value_string(str) { \
	string_status = STRING_append_string((line_data_read -> value_ptr), NODE_STRING_BUFFER_SIZE, str, &buffer_size); \
	STRING_check_status(NODE_ERROR_BASE_STRING); \
}

#define NODE_append_value_int32(value, format, prefix) { \
	char_t str[NODE_STRING_BUFFER_SIZE]; \
	string_status = STRING_value_to_string((int32_t) value, format, prefix, str); \
	STRING_check_status(NODE_ERROR_BASE_STRING); \
	NODE_append_value_string(str); \
}

#define NODE_flush_string_value(void) { \
	uint8_t char_idx = 0; \
	for (char_idx=0 ; char_idx<NODE_STRING_BUFFER_SIZE ; char_idx++) { \
		(line_data_read -> value_ptr)[char_idx] = STRING_CHAR_NULL; \
	} \
	buffer_size = 0; \
}

#define NODE_check_status(error_base) { if (node_status != NODE_SUCCESS) { status = error_base + node_status; goto errors; }}
#define NODE_stack_error() { ERROR_check_status(node_status, NODE_SUCCESS, ERROR_BASE_NODE); }
#define NODE_stack_error_print() { ERROR_check_status_print(node_status, NODE_SUCCESS, ERROR_BASE_NODE); }

#endif /* __NODE_H__ */
