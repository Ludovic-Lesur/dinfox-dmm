/*
 * node.c
 *
 *  Created on: 23 jan. 2023
 *      Author: Ludo
 */

#include "node.h"

#include "dinfox.h"
#include "lvrm.h"
#include "node_common.h"
#include "rs485.h"

/*** NODE structures ***/

typedef NODE_status_t (*NODE_get_board_name_t)(char_t** board_name_ptr);
typedef NODE_status_t (*NODE_set_rs485_address_t)(RS485_address_t rs485_address);
typedef NODE_status_t (*NODE_update_data_t)(uint8_t string_data_index);
typedef NODE_status_t (*NODE_update_all_data_t)(void);
typedef NODE_status_t (*NODE_read_string_data_t)(uint8_t string_data_index, char_t** string_data_name_ptr, char_t** string_data_value_ptr);
typedef NODE_status_t (*NODE_get_sigfox_payload_t)(uint8_t* ul_payload, uint8_t* ul_payload_size);
typedef NODE_status_t (*NODE_write_data_t)(uint8_t string_data_index, uint8_t value, RS485_reply_status_t* write_status);

typedef struct {
	NODE_get_board_name_t get_board_name;
	NODE_set_rs485_address_t set_rs485_address;
	NODE_update_data_t update_data;
	NODE_update_all_data_t update_all_data;
	NODE_read_string_data_t read_string_data;
	NODE_write_data_t write_data;
	NODE_get_sigfox_payload_t get_sigfox_payload;
} NODE_functions_t;

/*** NODE local global variables ***/

// Note: table is indexed with board ID.
static const NODE_functions_t NODE_FUNCTION[DINFOX_BOARD_ID_LAST] = {
	{&LVRM_get_board_name, &LVRM_set_rs485_address, &LVRM_update_data, &LVRM_update_all_data, &LVRM_read_string_data, &LVRM_write_data, &LVRM_get_sigfox_payload},
	{NULL, NULL, NULL, NULL, NULL, NULL},
	{NULL, NULL, NULL, NULL, NULL, NULL},
	{NULL, NULL, NULL, NULL, NULL, NULL},
	{NULL, NULL, NULL, NULL, NULL, NULL},
	{NULL, NULL, NULL, NULL, NULL, NULL},
	{NULL, NULL, NULL, NULL, NULL, NULL},
	{NULL, NULL, NULL, NULL, NULL, NULL},
	{NULL, NULL, NULL, NULL, NULL, NULL},
	{NULL, NULL, NULL, NULL, NULL, NULL},
};

/*** NODE local functions ***/

/* CHECK BOARD ID.
 * @param:	None.
 * @return:	None.
 */
#define _NODE_check_board_id() { \
	if ((node -> board_id) >= DINFOX_BOARD_ID_LAST) { \
		status = NODE_ERROR_BOARD_ID; \
		goto errors; \
	} \
}

/* CHECK FUNCTION POINTER.
 * @param function_name:	Function to check.
 * @return:					None.
 */
#define _NODE_check_function_pointer(function_name) { \
	if ((NODE_FUNCTION[node -> board_id].function_name) == NULL) { \
		status = NODE_ERROR_NOT_SUPPORTED; \
		goto errors; \
	} \
}

/*** NODE functions ***/

/* GET NODE BOARD NAME.
 * @param rs485_node:	Node to get name of.
 * @param board_name:	Pointer to string that will contain board name.
 * @return status:		Function execution status.
 */
NODE_status_t NODE_get_name(RS485_node_t* node, char_t** board_name) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	// Check board ID.
	_NODE_check_board_id();
	// Execute functions of the corresponding board ID.
	_NODE_check_function_pointer(get_board_name);
	status = NODE_FUNCTION[node -> board_id].get_board_name(board_name);
errors:
	return status;
}

/* PERFORM A SINGLE NODE MEASUREMENT.
 * @param node:					Node to update.
 * @param string_data_index:	Node string data index.
 * @return status:				Function execution status.
 */
NODE_status_t NODE_update_data(RS485_node_t* node, uint8_t string_data_index) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	// Check board ID.
	_NODE_check_board_id();
	// Execute functions of the corresponding board ID.
	_NODE_check_function_pointer(set_rs485_address);
	status = NODE_FUNCTION[node -> board_id].set_rs485_address(node -> address);
	if (status != NODE_SUCCESS) goto errors;
	_NODE_check_function_pointer(update_data);
	status = NODE_FUNCTION[node -> board_id].update_data(string_data_index);
errors:
	return status;
}

/* PERFORM ALL NODE MEASUREMENTS.
 * @param node:		Node to update.
 * @return status:	Function execution status.
 */
NODE_status_t NODE_update_all_data(RS485_node_t* node) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	// Check board ID.
	_NODE_check_board_id();
	// Execute functions of the corresponding board ID.
	_NODE_check_function_pointer(set_rs485_address);
	status = NODE_FUNCTION[node -> board_id].set_rs485_address(node -> address);
	if (status != NODE_SUCCESS) goto errors;
	_NODE_check_function_pointer(update_all_data);
	status = NODE_FUNCTION[node -> board_id].update_all_data();
errors:
	return status;
}

/* UNSTACK NODE DATA FORMATTED AS STRING.
 * @param node:						Node to read.
 * @param string_data_index:		Node string data index.
 * @param string_data_name_ptr:		Pointer to string that will contain next measurement name.
 * @param string_data_value_ptr:	Pointer to string that will contain next measurement value.
 * @return status:					Function execution status.
 */
NODE_status_t NODE_read_string_data(RS485_node_t* node, uint8_t string_data_index, char_t** string_data_name_ptr, char_t** string_data_value_ptr) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	// Check board ID.
	_NODE_check_board_id();
	// Execute functions of the corresponding board ID.
	_NODE_check_function_pointer(set_rs485_address);
	status = NODE_FUNCTION[node -> board_id].set_rs485_address(node -> address);
	if (status != NODE_SUCCESS) goto errors;
	_NODE_check_function_pointer(read_string_data);
	status = NODE_FUNCTION[node -> board_id].read_string_data(string_data_index, string_data_name_ptr, string_data_value_ptr);
errors:
	return status;
}

/* WRITE NODE DATA.
 * @param node:					Node to write.
 * @param string_data_index:	Node string data index.
 * @param value:				Value to write in corresponding register.
 * @param write_status:			Pointer to the writing operation status.
 * @return status:				Function execution status.
 */
NODE_status_t NODE_write_data(RS485_node_t* node, uint8_t string_data_index, uint8_t value, RS485_reply_status_t* write_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	// Check board ID.
	_NODE_check_board_id();
	// Execute function of the corresponding board ID.
	_NODE_check_function_pointer(set_rs485_address);
	status = NODE_FUNCTION[node -> board_id].set_rs485_address(node -> address);
	if (status != NODE_SUCCESS) goto errors;
	_NODE_check_function_pointer(write_data);
	status = NODE_FUNCTION[node -> board_id].write_data(string_data_index, value, write_status);
errors:
	return status;
}

/* GET NODE SIGFOX PAYLOAD.
 * @param node:				Node to read.
 * @param ul_payload:		Pointer that will contain UL payload of the node.
 * @param ul_payload_size:	Pointer to byte that will contain UL payload size.
 * @return status:			Function execution status.
 */
NODE_status_t NODE_get_sigfox_payload(RS485_node_t* node, uint8_t* ul_payload, uint8_t* ul_payload_size) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	// Check board ID.
	_NODE_check_board_id();
	// Execute function of the corresponding board ID.
	_NODE_check_function_pointer(set_rs485_address);
	status = NODE_FUNCTION[node -> board_id].set_rs485_address(node -> address);
	if (status != NODE_SUCCESS) goto errors;
	_NODE_check_function_pointer(get_sigfox_payload);
	status = NODE_FUNCTION[node -> board_id].get_sigfox_payload(ul_payload, ul_payload_size);
errors:
	return status;
}
