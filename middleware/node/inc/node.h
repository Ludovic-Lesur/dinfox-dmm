/*
 * node.h
 *
 *  Created on: 22 jan 2023
 *      Author: Ludo
 */

#ifndef __NODE_H__
#define __NODE_H__

#include "adc.h"
#include "lbus.h"
#include "lptim.h"
#include "lpuart.h"
#include "node_common.h"
#include "power.h"
#include "sigfox_types.h"
#include "string.h"
#include "types.h"

/*** NODES macros ***/

#define NODES_LIST_SIZE_MAX					32
#define NODE_STRING_BUFFER_SIZE				32

#define NODE_DINFOX_PAYLOAD_HEADER_SIZE		2

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

/*** NODE global variables ***/

extern const char_t NODE_ERROR_STRING[];

/*** NODE structures ***/

/*!******************************************************************
 * \enum NODE_status_t
 * \brief NODE driver error codes.
 *******************************************************************/
typedef enum {
	// Driver errors.
	NODE_SUCCESS = 0,
	NODE_ERROR_NOT_SUPPORTED,
	NODE_ERROR_NULL_PARAMETER,
	NODE_ERROR_WRITE_ACCESS,
	NODE_ERROR_READ_ACCESS,
	NODE_ERROR_RADIO_SEND_DATA,
	NODE_ERROR_RADIO_READ_DATA,
	NODE_ERROR_RADIO_READ_BIDIRECTIONAL_MC,
	NODE_ERROR_RADIO_NONE_MODULE,
	NODE_ERROR_PROTOCOL,
	NODE_ERROR_R4S8CR_ADDRESS,
	NODE_ERROR_MPMCM_ADDRESS,
	NODE_ERROR_REGISTER_ADDRESS,
	NODE_ERROR_REGISTER_FORMAT,
	NODE_ERROR_REGISTER_READ_ONLY,
	NODE_ERROR_REGISTER_FIELD_RANGE,
	NODE_ERROR_REGISTER_LIST_SIZE,
	NODE_ERROR_LINE_DATA_INDEX,
	NODE_ERROR_REPLY_TYPE,
	NODE_ERROR_SIGFOX_UL_PAYLOAD_TYPE,
	NODE_ERROR_SIGFOX_UPLINK_PERIOD,
	NODE_ERROR_SIGFOX_DOWNLINK_PERIOD,
	NODE_ERROR_SIGFOX_DOWNLINK_NODE_ADDRESS,
	NODE_ERROR_SIGFOX_DOWNLINK_OPERATION_CODE,
	NODE_ERROR_ACTIONS_LIST_INDEX,
	NODE_ERROR_ACTIONS_LIST_OVERFLOW,
	NODE_ERROR_RELAY_ID,
	// Low level drivers errors.
	NODE_ERROR_BASE_ACCESS_STATUS_CODE = 0x0100,
	NODE_ERROR_BASE_ACCESS_STATUS_ADDRESS = 0x0200,
	NODE_ERROR_BASE_ADC1 = (NODE_ERROR_BASE_ACCESS_STATUS_ADDRESS + 0x0100),
	NODE_ERROR_BASE_LPUART1 = (NODE_ERROR_BASE_ADC1 + ADC_ERROR_BASE_LAST),
	NODE_ERROR_BASE_LPTIM1 = (NODE_ERROR_BASE_LPUART1 + LPUART_ERROR_BASE_LAST),
	NODE_ERROR_BASE_STRING = (NODE_ERROR_BASE_LPTIM1 + LPTIM_ERROR_BASE_LAST),
	NODE_ERROR_BASE_POWER = (NODE_ERROR_BASE_STRING + STRING_ERROR_BASE_LAST),
	NODE_ERROR_BASE_LBUS = (NODE_ERROR_BASE_POWER + POWER_ERROR_BASE_LAST),
	// Last base value.
	NODE_ERROR_BASE_LAST = (NODE_ERROR_BASE_LBUS + LBUS_ERROR_BASE_LAST)
} NODE_status_t;

/*!******************************************************************
 * \enum NODE_t
 * \brief Node descriptor.
 *******************************************************************/
typedef struct {
	NODE_address_t address;
	uint8_t board_id;
	uint8_t radio_transmission_count;
} NODE_t;

/*!******************************************************************
 * \enum NODE_list_t
 * \brief Node list type.
 *******************************************************************/
typedef struct {
	NODE_t list[NODES_LIST_SIZE_MAX];
	uint8_t count;
} NODE_list_t;

/*!******************************************************************
 * \enum NODE_action_t
 * \brief Node action type.
 *******************************************************************/
typedef struct {
	NODE_t* node;
	uint32_t downlink_hash;
	uint32_t timestamp_seconds;
	uint8_t reg_addr;
	uint32_t reg_value;
	uint32_t reg_mask;
	NODE_access_status_t access_status;
} NODE_action_t;

/*!******************************************************************
 * \enum NODE_line_data_t
 * \brief Displayed line data of a node.
 *******************************************************************/
typedef struct {
	char_t* name;
	char_t* unit;
	STRING_format_t print_format;
	uint8_t print_prefix;
	uint8_t read_reg_addr;
	uint32_t read_field_mask;
	uint8_t write_reg_addr;
	uint32_t write_field_mask;
} NODE_line_data_t;

/*!******************************************************************
 * \enum NODE_line_data_write_t
 * \brief Line data write parameters.
 *******************************************************************/
typedef struct {
	NODE_address_t node_addr;
	uint8_t line_data_index;
	uint32_t field_value;
} NODE_line_data_write_t;

/*!******************************************************************
 * \enum NODE_line_data_read_t
 * \brief Line data read parameters.
 *******************************************************************/
typedef struct {
	NODE_address_t node_addr;
	uint8_t line_data_index;
	char_t* name_ptr;
	char_t* value_ptr;
} NODE_line_data_read_t;

/*!******************************************************************
 * \enum NODE_ul_payload_t
 * \brief Node UL payload structure.
 *******************************************************************/
typedef struct {
	NODE_t* node;
	uint8_t* ul_payload;
	uint8_t* size;
} NODE_ul_payload_t;

/*!******************************************************************
 * \enum NODE_dinfox_ul_payload_t
 * \brief Sigfox UL payload structure.
 *******************************************************************/
typedef union {
	uint8_t frame[SIGFOX_UL_PAYLOAD_MAX_SIZE_BYTES];
	struct {
		unsigned node_addr : 8;
		unsigned board_id : 8;
		uint8_t node_data[SIGFOX_UL_PAYLOAD_MAX_SIZE_BYTES - NODE_DINFOX_PAYLOAD_HEADER_SIZE];
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} NODE_dinfox_ul_payload_t;

/*** NODES global variables ***/

extern NODE_list_t NODES_LIST;

/*** NODE functions ***/

/*!******************************************************************
 * \fn void NODE_init_por(void)
 * \brief POR initialization of the node interface.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		none
 *******************************************************************/
void NODE_init_por(void);

/*!******************************************************************
 * \fn void NODE_init(void)
 * \brief Init node interface.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		none
 *******************************************************************/
void NODE_init(void);

/*!******************************************************************
 * \fn void NODE_de_init(void)
 * \brief Release node interface.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		none
 *******************************************************************/
void NODE_de_init(void);

/*!******************************************************************
 * \fn NODE_status_t NODE_scan(void)
 * \brief Scan all nodes connected to the RS485 bus.
 * \param[in]  	none
 * \param[out]	none
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t NODE_scan(void);

/*!******************************************************************
 * \fn void NODE_process(void)
 * \brief Main task of node interface.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		none
 *******************************************************************/
void NODE_process(void);

/*!******************************************************************
 * \fn NODE_status_t NODE_write_line_data(NODE_t* node, uint8_t line_data_index, uint32_t field_value)
 * \brief Write corresponding node register of screen data line.
 * \param[in]  	node: Pointer to the node.
 * \param[in]	line_data_index: Index of the data line to write.
 * \param[in]	field_value: Value to write in corresponding register field.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t NODE_write_line_data(NODE_t* node, uint8_t line_data_index, uint32_t field_value);

/*!******************************************************************
 * \fn NODE_status_t NODE_read_line_data(NODE_t* node, uint8_t line_data_index)
 * \brief Read corresponding node register of screen data line.
 * \param[in]  	node: Pointer to the node.
 * \param[in]	line_data_index: Index of the data line to read.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t NODE_read_line_data(NODE_t* node, uint8_t line_data_index);

/*!******************************************************************
 * \fn NODE_status_t NODE_read_line_data_all(NODE_t* node)
 * \brief Read corresponding node registers of all screen data lines.
 * \param[in]  	node: Pointer to the node.
 * \param[out] 	none
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t NODE_read_line_data_all(NODE_t* node);

/*!******************************************************************
 * \fn NODE_status_t NODE_get_name(NODE_t* node, char_t** board_name)
 * \brief Get the name of a node.
 * \param[in]  	node: Pointer to the node.
 * \param[out] 	board_name: Pointer to string that will contain the board name.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t NODE_get_name(NODE_t* node, char_t** board_name);

/*!******************************************************************
 * \fn NODE_status_t NODE_get_last_line_data_index(NODE_t* node, uint8_t* last_line_data_index)
 * \brief Get the number of data lines.
 * \param[in]  	node: Pointer to the node.
 * \param[out] 	last_line_data_index: Pointer to byte that will contain the number of displayed data lines.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t NODE_get_last_line_data_index(NODE_t* node, uint8_t* last_line_data_index);

/*!******************************************************************
 * \fn NODE_status_t NODE_get_line_data(NODE_t* node, uint8_t line_data_index, char_t** line_data_name_ptr, char_t** line_data_value_ptr)
 * \brief Read data line.
 * \param[in]  	node: Pointer to the node.
 * \param[in]	line_data_index: Index of the data line to read.
 * \param[out] 	line_data_name_ptr: Pointer to string that will contain the data name.
 * \param[out] 	line_data_value_ptr: Pointer to string that will contain the data value.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t NODE_get_line_data(NODE_t* node, uint8_t line_data_index, char_t** line_data_name_ptr, char_t** line_data_value_ptr);

/*******************************************************************/
#define NODE_append_name_string(str) { \
	string_status = STRING_append_string((line_data_read -> name_ptr), NODE_STRING_BUFFER_SIZE, str, &buffer_size); \
	STRING_exit_error(NODE_ERROR_BASE_STRING); \
}

/*******************************************************************/
#define NODE_append_value_string(str) { \
	string_status = STRING_append_string((line_data_read -> value_ptr), NODE_STRING_BUFFER_SIZE, str, &buffer_size); \
	STRING_exit_error(NODE_ERROR_BASE_STRING); \
}

/*******************************************************************/
#define NODE_append_value_int32(value, format, prefix) { \
	char_t str[NODE_STRING_BUFFER_SIZE]; \
	string_status = STRING_value_to_string((int32_t) value, format, prefix, str); \
	STRING_exit_error(NODE_ERROR_BASE_STRING); \
	NODE_append_value_string(str); \
}

/*******************************************************************/
#define NODE_flush_string_value(void) { \
	uint8_t char_idx = 0; \
	for (char_idx=0 ; char_idx<NODE_STRING_BUFFER_SIZE ; char_idx++) { \
		(line_data_read -> value_ptr)[char_idx] = STRING_CHAR_NULL; \
	} \
	buffer_size = 0; \
}

/*******************************************************************/
#define NODE_exit_error(error_base) { if (node_status != NODE_SUCCESS) { status = (error_base + node_status); goto errors; } }

/*******************************************************************/
#define NODE_stack_error(void) { if (node_status != NODE_SUCCESS) { ERROR_stack_add(ERROR_BASE_NODE + node_status); } }

/*******************************************************************/
#define NODE_stack_exit_error(error_code) { if (node_status != NODE_SUCCESS) { ERROR_stack_add(ERROR_BASE_NODE + node_status); status = error_code; goto errors; } }

#endif /* __NODE_H__ */
