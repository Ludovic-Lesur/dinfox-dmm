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

/*** NODE local macros ***/

#define NODE_STRING_DATA_INDEX_MAX	32
#define NODE_REGISTER_ADDRESS_MAX	64

/*** NODE local structures ***/

typedef struct {
	char_t* name;
	uint8_t last_register_address;
	uint8_t last_string_data_index;
	STRING_format_t* registers_format;
	NODE_functions_t functions;
} NODE_t;

typedef struct {
	char_t string_data_name[NODE_STRING_DATA_INDEX_MAX][NODE_STRING_BUFFER_SIZE];
	char_t string_data_value[NODE_STRING_DATA_INDEX_MAX][NODE_STRING_BUFFER_SIZE];
	int32_t integer_data_value[NODE_REGISTER_ADDRESS_MAX];
} NODE_data_t;

/*** NODE local global variables ***/

// Note: table is indexed with board ID.
static const NODE_t NODES[DINFOX_BOARD_ID_LAST] = {
	{"LVRM", LVRM_REGISTER_LAST, LVRM_STRING_DATA_INDEX_LAST, (STRING_format_t*) &LVRM_REGISTERS_FORMAT, {&LVRM_update_specific_data, &LVRM_get_sigfox_payload}},
};
static NODE_data_t node_ctx;

/*** NODE local functions ***/

/* CHECK NODE POINTER AND BOARD ID.
 * @param:	None.
 * @return:	None.
 */
#define _NODE_check_node_and_board_id(void) { \
	if (node == NULL) { \
		status = NODE_ERROR_NULL_PARAMETER; \
		goto errors; \
	} \
	if ((node -> board_id) >= DINFOX_BOARD_ID_LAST) { \
		status = NODE_ERROR_NOT_SUPPORTED; \
		goto errors; \
	} \
}

/* CHECK FUNCTION POINTER.
 * @param function_name:	Function to check.
 * @return:					None.
 */
#define _NODE_check_function_pointer(function_name) { \
	if ((NODES[node -> board_id].functions.function_name) == NULL) { \
		status = NODE_ERROR_NOT_SUPPORTED; \
		goto errors; \
	} \
}

/* GENERIC MACRO TO APPEND A STRING TO THE DATAS VALUE BUFFER.
 * @param measurement_idx:	Measurement line index.
 * @param str:				String to append.
 * @return:					None.
 */
#define _NODE_append_string(str) { \
	string_status = STRING_append_string(node_ctx.string_data_value[string_data_index], DINFOX_STRING_BUFFER_SIZE, str, &(node_ctx.string_data_value_size[string_data_index])); \
	STRING_status_check(NODE_ERROR_BASE_STRING); \
}

/* GENERIC MACRO TO SET A DATA VALUE TO ERROR.
 * @param measurement_idx:	Measurement line index.
 * @return:					None.
 */
#define _NODE_set_error() { \
	_NODE_flush_string_data_value(string_data_index); \
	_NODE_append_string(NODE_STRING_DATA_ERROR); \
}

/* FLUSH ONE LINE OF THE MEASURERMENTS VALUE BUFFER.
 * @param:	None.
 * @return:	None.
 */
static void _NODE_flush_string_data_value(uint8_t string_data_index) {
	// Local variables.
	uint8_t idx = 0;
	// Char loop.
	for (idx=0 ; idx<NODE_STRING_BUFFER_SIZE ; idx++) node_ctx.string_data_value[string_data_index][idx] = STRING_CHAR_NULL;
}

/* FLUSH WHOLE DATAS VALUE BUFFER.
 * @param:	None.
 * @return:	None.
 */
void _NODE_flush_all_data_value(void) {
	// Local variables.
	uint8_t idx = 0;
	// String data.
	for (idx=0 ; idx<NODE_STRING_DATA_INDEX_MAX ; idx++) _NODE_flush_string_data_value(idx);
	// Integer data.
	for (idx=0 ; idx<NODE_REGISTER_ADDRESS_MAX ; idx++) node_ctx.integer_data_value[idx] = 0;
}

/*** NODE functions ***/

/* GET NODE BOARD NAME.
 * @param rs485_node:		Node to get name of.
 * @param board_name_ptr:	Pointer to string that will contain board name.
 * @return status:			Function execution status.
 */
NODE_status_t NODE_get_name(RS485_node_t* node, char_t** board_name_ptr) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	// Check board ID.
	_NODE_check_node_and_board_id();
	// Get name of the corresponding board ID.
	(*board_name_ptr) = (char_t*) NODES[node -> board_id].name;
errors:
	return status;
}

/* GET NODE LAST STRING INDEX.
 * @param rs485_node:				Node to get name of.
 * @param last_string_data_index:	Pointer to byte that will contain last string index.
 * @return status:				Function execution status.
 */
NODE_status_t NODE_get_last_string_data_index(RS485_node_t* node, uint8_t* last_string_data_index) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	// Check board ID.
	_NODE_check_node_and_board_id();
	// Get name of the corresponding board ID.
	(*last_string_data_index) = NODES[node -> board_id].last_string_data_index;
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
	NODE_single_data_ptr_t single_data_ptr;
	// Check board ID.
	_NODE_check_node_and_board_id();
	_NODE_check_function_pointer(update_specific_data);
	// Flush line.
	_NODE_flush_string_data_value(string_data_index);
	// Update pointers.
	single_data_ptr.string_name_ptr = (char_t*) &(node_ctx.string_data_name[string_data_index]);
	single_data_ptr.string_value_ptr = (char_t*) &(node_ctx.string_data_value[string_data_index]);
	single_data_ptr.value_ptr = &(node_ctx.integer_data_value[string_data_index]);
	// Check index.
	if (string_data_index < DINFOX_STRING_DATA_INDEX_LAST) {
		// Common data.
		status = DINFOX_update_common_data((node -> address), string_data_index, &single_data_ptr);
	}
	else {
		// Specific data.
		status = NODES[node -> board_id].functions.update_specific_data((node -> address), string_data_index, &single_data_ptr);
	}
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
	uint8_t idx = 0;
	// Check board ID.
	_NODE_check_node_and_board_id();
	// Reset buffers.
	_NODE_flush_all_data_value();
	// String data loop.
	for (idx=0 ; idx<(NODES[node -> board_id].last_string_data_index) ; idx++) {
		status = NODE_update_data(node, idx);
		if (status != NODE_SUCCESS) goto errors;
	}
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
	// Check parameters.
	_NODE_check_node_and_board_id();
	// Check index.
	if (NODES[node -> board_id].last_string_data_index == 0) {
		status = NODE_ERROR_NOT_SUPPORTED;
		goto errors;
	}
	if (string_data_index >= (NODES[node -> board_id].last_string_data_index)) { \
		status = NODE_ERROR_STRING_DATA_INDEX;
		goto errors;
	}
	// Update pointers.
	(*string_data_name_ptr) = (char_t*) node_ctx.string_data_name[string_data_index];
	(*string_data_value_ptr) = (char_t*) node_ctx.string_data_value[string_data_index];
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
NODE_status_t NODE_write_register(RS485_node_t* node, uint8_t register_address, int32_t value, RS485_reply_status_t* write_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	RS485_status_t rs485_status = RS485_SUCCESS;
	RS485_write_input_t write_input;
	RS485_reply_t reply;
	// Check node and board ID.
	_NODE_check_node_and_board_id();
	// Check write status.
	if (write_status == NULL) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Check register address.
	if (NODES[node -> board_id].last_register_address == 0) {
		status = NODE_ERROR_NOT_SUPPORTED;
		goto errors;
	}
	if (register_address >= (NODES[node -> board_id].last_register_address)) {
		status = NODE_ERROR_REGISTER_ADDRESS;
		goto errors;
	}
	// Build write input parameters.
#ifdef AM
	write_input.node_address = (node -> address);
#endif
	write_input.value = value;
	write_input.timeout_ms = NODE_RS485_TIMEOUT_MS;
	write_input.format = (register_address < DINFOX_REGISTER_LAST) ? DINFOX_REGISTERS_FORMAT[register_address] : NODES[node -> board_id].registers_format[register_address - DINFOX_REGISTER_LAST];
	write_input.register_address = register_address;
	// Check writable registers.
	rs485_status = RS485_write_register(&write_input, &reply);
	RS485_status_check(NODE_ERROR_BASE_RS485);
	// Check reply.
	(*write_status).all = reply.status.all;
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
NODE_status_t NODE_write_string_data(RS485_node_t* node, uint8_t string_data_index, int32_t value, RS485_reply_status_t* write_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	uint8_t register_address = 0;
	// Convert string data index to register.
	if (string_data_index >= DINFOX_STRING_DATA_INDEX_LAST) {
		register_address = (string_data_index + DINFOX_REGISTER_LAST - DINFOX_STRING_DATA_INDEX_LAST);
	}
	// Write register.
	status = NODE_write_register(node, register_address, value, write_status);
	return status;
}

/* GET NODE SIGFOX PAYLOAD.
 * @param node:				Node to read.
 * @param ul_payload:		Pointer that will contain UL payload of the node.
 * @param ul_payload_size:	Pointer to byte that will contain UL payload size.
 * @return status:			Function execution status.
 */
NODE_status_t NODE_get_sigfox_payload(NODE_sigfox_payload_type_t sigfox_payload_type, RS485_node_t* node, uint8_t* ul_payload, uint8_t* ul_payload_size) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	// Check board ID.
	_NODE_check_node_and_board_id();
	// Check payload type.
	switch (sigfox_payload_type) {
	case NODE_SIGFOX_PAYLOAD_TYPE_STARTUP:
		// TODO build startup data here since the format is common to all boards.
		break;
	case NODE_SIGFOX_PAYLOAD_TYPE_DATA:
	case NODE_SIGFOX_PAYLOAD_TYPE_MONITORING:
		_NODE_check_function_pointer(get_sigfox_payload);
		status = NODES[node -> board_id].functions.get_sigfox_payload(sigfox_payload_type, ul_payload, ul_payload_size);
		break;
	default:
		status = NODE_ERROR_SIGFOX_PAYLOAD_TYPE;
		goto errors;
	}
	// Execute function of the corresponding board ID.
errors:
	return status;
}
