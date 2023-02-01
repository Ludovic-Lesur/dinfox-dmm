/*
 * node_common.h
 *
 *  Created on: 23 jan. 2023
 *      Author: Ludo
 */

#ifndef __NODE_COMMON_H__
#define __NODE_COMMON_H__

#include "rs485.h"
#include "string.h"

/*** NODE common macros ***/

#define NODE_STRING_BUFFER_SIZE		16
#define NODE_STRING_DATA_ERROR		"ERROR"

#define NODE_RS485_TIMEOUT_MS		100

/*** NODE common structures ***/

typedef enum {
	NODE_SUCCESS = 0,
	NODE_ERROR_NOT_SUPPORTED,
	NODE_ERROR_NULL_PARAMETER,
	NODE_ERROR_RS485_ADDRESS,
	NODE_ERROR_REGISTER_ADDRESS,
	NODE_ERROR_STRING_DATA_INDEX,
	NODE_ERROR_SIGFOX_PAYLOAD_TYPE,
	NODE_ERROR_BASE_STRING = 0x0100,
	NODE_ERROR_BASE_RS485 = (NODE_ERROR_BASE_STRING + STRING_ERROR_BASE_LAST),
	NODE_ERROR_BASE_LAST = (NODE_ERROR_BASE_RS485 + RS485_ERROR_BASE_LAST)
} NODE_status_t;

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

/*** NODE common functions structure ***/

typedef NODE_status_t (*NODE_update_specific_data_t)(RS485_address_t rs485_address, uint8_t string_data_index, NODE_single_data_ptr_t* single_data_ptr);
typedef NODE_status_t (*NODE_get_sigfox_payload_t)(NODE_sigfox_payload_type_t sigfox_payload_type, uint8_t* ul_payload, uint8_t* ul_payload_size);

typedef struct {
	NODE_update_specific_data_t update_specific_data;
	NODE_get_sigfox_payload_t get_sigfox_payload;
} NODE_functions_t;

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
