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

/*** NODE structures ***/

typedef NODE_status_t (*NODE_get_board_name_t)(char_t** board_name_ptr);
typedef NODE_status_t (*NODE_set_rs485_address_t)(RS485_address_t rs485_address);
typedef NODE_status_t (*NODE_perform_measurements_t)(void);
typedef NODE_status_t (*NODE_unstack_string_data_t)(char_t** measurements_name_ptr, char_t** measurements_value_ptr);
typedef NODE_status_t (*NODE_get_sigfox_payload_t)(uint8_t* ul_payload, uint8_t* ul_payload_size);
typedef NODE_status_t (*NODE_write_t)(uint8_t register_address, uint8_t value);

typedef struct {
	NODE_get_board_name_t get_board_name;
	NODE_set_rs485_address_t set_rs485_address;
	NODE_perform_measurements_t perform_measurements;
	NODE_unstack_string_data_t unstack_string_data;
	NODE_get_sigfox_payload_t get_sigfox_payload;
	NODE_write_t write;
} NODE_functions_t;

/*** NODE local global variables ***/

// Note: table is indexed with board ID.
static const NODE_functions_t NODE_FUNCTION[DINFOX_BOARD_ID_LAST] = {
	{&LVRM_get_board_name, &LVRM_set_rs485_address, &LVRM_perform_measurements, &LVRM_unstack_string_data, &LVRM_get_sigfox_payload, &LVRM_write},
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
	if ((rs485_node -> board_id) >= DINFOX_BOARD_ID_LAST) { \
		status = NODE_ERROR_BOARD_ID; \
		goto errors; \
	} \
}

/* CHECK FUNCTION POINTER.
 * @param function_name:	Function to check.
 * @return:					None.
 */
#define _NODE_check_function_pointer(function_name) { \
	if ((NODE_FUNCTION[rs485_node -> board_id].function_name) == NULL) { \
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
NODE_status_t NODE_get_name(RS485_node_t* rs485_node, char_t** board_name) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	// Check board ID.
	_NODE_check_board_id();
	// Execute functions of the corresponding board ID.
	_NODE_check_function_pointer(get_board_name);
	status = NODE_FUNCTION[rs485_node -> board_id].get_board_name(board_name);
errors:
	return status;
}

/* PERFORM NODE MEASUREMENTS.
 * @param node:		Node to analyze.
 * @return status:	Function execution status.
 */
NODE_status_t NODE_perform_measurements(RS485_node_t* rs485_node) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	// Check board ID.
	_NODE_check_board_id();
	// Execute functions of the corresponding board ID.
	_NODE_check_function_pointer(set_rs485_address);
	status = NODE_FUNCTION[rs485_node -> board_id].set_rs485_address(rs485_node -> address);
	if (status != NODE_SUCCESS) goto errors;
	_NODE_check_function_pointer(perform_measurements);
	status = NODE_FUNCTION[rs485_node -> board_id].perform_measurements();
errors:
	return status;
}

/* UNSTACK NODE DATA FORMATTED AS STRING.
 * @param node:						Node to read.
 * @param measurement_name_ptr:		Pointer to string that will contain next measurement name.
 * @param measurement_value_ptr:	Pointer to string that will contain next measurement value.
 * @return status:					Function execution status.
 */
NODE_status_t NODE_unstack_string_data(RS485_node_t* rs485_node, char_t** measurements_name_ptr, char_t** measurements_value_ptr) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	// Check board ID.
	_NODE_check_board_id();
	// Execute functions of the corresponding board ID.
	_NODE_check_function_pointer(set_rs485_address);
	status = NODE_FUNCTION[rs485_node -> board_id].set_rs485_address(rs485_node -> address);
	if (status != NODE_SUCCESS) goto errors;
	_NODE_check_function_pointer(unstack_string_data);
	status = NODE_FUNCTION[rs485_node -> board_id].unstack_string_data(measurements_name_ptr, measurements_value_ptr);
errors:
	return status;
}

/* GET NODE SIGFOX PAYLOAD.
 * @param node:				Node to read.
 * @param ul_payload:		Pointer that will contain UL payload of the node.
 * @param ul_payload_size:	Pointer to byte that will contain UL payload size.
 * @return status:			Function execution status.
 */
NODE_status_t NODE_get_sigfox_payload(RS485_node_t* rs485_node, uint8_t* ul_payload, uint8_t* ul_payload_size) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	// Check board ID.
	_NODE_check_board_id();
	// Execute function of the corresponding board ID.
	_NODE_check_function_pointer(set_rs485_address);
	status = NODE_FUNCTION[rs485_node -> board_id].set_rs485_address(rs485_node -> address);
	if (status != NODE_SUCCESS) goto errors;
	_NODE_check_function_pointer(get_sigfox_payload);
	status = NODE_FUNCTION[rs485_node -> board_id].get_sigfox_payload(ul_payload, ul_payload_size);
errors:
	return status;
}

/* WRITE NODE DATA.
 * @param node:				Node to write.
 * @param register_address:	Register to write.
 * @param value:			Value to write in register.
 * @return status:			Function execution status.
 */
NODE_status_t NODE_write(RS485_node_t* rs485_node, uint8_t register_address, uint8_t value) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	// Check board ID.
	_NODE_check_board_id();
	// Execute function of the corresponding board ID.
	_NODE_check_function_pointer(set_rs485_address);
	status = NODE_FUNCTION[rs485_node -> board_id].set_rs485_address(rs485_node -> address);
	if (status != NODE_SUCCESS) goto errors;
	_NODE_check_function_pointer(write);
	status = NODE_FUNCTION[rs485_node -> board_id].write(register_address, value);
errors:
	return status;
}
