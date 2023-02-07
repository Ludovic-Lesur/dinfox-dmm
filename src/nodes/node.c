/*
 * node.c
 *
 *  Created on: 23 jan. 2023
 *      Author: Ludo
 */

#include "node.h"

#include "dinfox.h"
#include "lbus.h"
#include "lvrm.h"
#include "node_common.h"
#include "node_status.h"
#include "r4s8cr.h"

/*** NODE local macros ***/

#define NODE_STRING_DATA_INDEX_MAX	32
#define NODE_REGISTER_ADDRESS_MAX	64

/*** NODE local structures ***/

typedef enum {
	NODE_PROTOCOL_LBUS = 0,
	NODE_PROTOCOL_R4S8CR,
	NODE_PROTOCOL_LAST
} NODE_protocol_t;

typedef NODE_status_t (*NODE_read_register_t)(NODE_read_parameters_t* read_params, NODE_read_data_t* read_data, NODE_access_status_t* read_status);
typedef NODE_status_t (*NODE_write_register_t)(NODE_write_parameters_t* write_params, NODE_access_status_t* write_status);
typedef NODE_status_t (*NODE_update_data_t)(NODE_address_t rs485_address, uint8_t string_data_index, NODE_single_data_ptr_t* single_data_ptr);
typedef NODE_status_t (*NODE_get_sigfox_payload_t)(NODE_sigfox_payload_type_t payload_type, uint8_t* ul_payload, uint8_t* ul_payload_size);

typedef struct {
	NODE_read_register_t read_register;
	NODE_write_register_t write_register;
	NODE_update_data_t update_data;
	NODE_get_sigfox_payload_t get_sigfox_payload;
} NODE_functions_t;

typedef struct {
	char_t* name;
	NODE_protocol_t protocol;
	uint8_t last_register_address;
	uint8_t last_string_data_index;
	STRING_format_t* registers_format;
	NODE_functions_t functions;
} NODE_descriptor_t;

typedef struct {
	char_t string_data_name[NODE_STRING_DATA_INDEX_MAX][NODE_STRING_BUFFER_SIZE];
	char_t string_data_value[NODE_STRING_DATA_INDEX_MAX][NODE_STRING_BUFFER_SIZE];
	int32_t integer_data_value[NODE_REGISTER_ADDRESS_MAX];
} NODE_data_t;

typedef struct {
	NODE_data_t data;
} NODE_context_t;

/*** NODE local global variables ***/

// Note: table is indexed with board ID.
static const NODE_descriptor_t NODES[DINFOX_BOARD_ID_LAST] = {
	{"LVRM", NODE_PROTOCOL_LBUS, LVRM_REGISTER_LAST, LVRM_STRING_DATA_INDEX_LAST, (STRING_format_t*) LVRM_REGISTERS_FORMAT,
		{&LBUS_read_register, &LBUS_write_register, &LVRM_update_data, &LVRM_get_sigfox_payload}
	},
	{"BPSM", NODE_PROTOCOL_LBUS, 0, 0, NULL,
		{&LBUS_read_register, &LBUS_write_register, NULL, NULL}
	},
	{"DDRM", NODE_PROTOCOL_LBUS, 0, 0, NULL,
		{&LBUS_read_register, &LBUS_write_register, NULL, NULL}
	},
	{"UHFM", NODE_PROTOCOL_LBUS, 0, 0, NULL,
		{&LBUS_read_register, &LBUS_write_register, NULL, NULL}
	},
	{"GPSM", NODE_PROTOCOL_LBUS, 0, 0, NULL,
		{&LBUS_read_register, &LBUS_write_register, NULL, NULL}
	},
	{"SM", NODE_PROTOCOL_LBUS, 0, 0, NULL,
		{&LBUS_read_register, &LBUS_write_register, NULL, NULL}
	},
	{"DIM", NODE_PROTOCOL_LBUS, 0, 0, NULL,
		{&LBUS_read_register, &LBUS_write_register, NULL, NULL}
	},
	{"RRM", NODE_PROTOCOL_LBUS, 0, 0, NULL,
		{&LBUS_read_register, &LBUS_write_register, NULL, NULL}
	},
	{"DMM", NODE_PROTOCOL_LBUS, 0, 0, NULL,
		{&LBUS_read_register, &LBUS_write_register, NULL, NULL}
	},
	{"MPMCM", NODE_PROTOCOL_LBUS, 0, 0, NULL,
		{&LBUS_read_register, &LBUS_write_register, NULL, NULL}
	},
	{"R4S8CR", NODE_PROTOCOL_R4S8CR, R4S8CR_REGISTER_LAST, R4S8CR_STRING_DATA_INDEX_LAST, (STRING_format_t*) R4S8CR_REGISTERS_FORMAT,
		{&R4S8CR_read_register, &R4S8CR_write_register, &R4S8CR_update_data, &R4S8CR_get_sigfox_payload}},
};
static NODE_context_t node_ctx;

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

/* FLUSH ONE LINE OF THE MEASURERMENTS VALUE BUFFER.
 * @param:	None.
 * @return:	None.
 */
static void _NODE_flush_string_data_value(uint8_t string_data_index) {
	// Local variables.
	uint8_t idx = 0;
	// Char loop.
	for (idx=0 ; idx<NODE_STRING_BUFFER_SIZE ; idx++) {
		node_ctx.data.string_data_name[string_data_index][idx] = STRING_CHAR_NULL;
		node_ctx.data.string_data_value[string_data_index][idx] = STRING_CHAR_NULL;
	}
}

/* FLUSH WHOLE DATAS VALUE BUFFER.
 * @param:	None.
 * @return:	None.
 */
void _NODE_flush_all_data_value(void) {
	// Local variables.
	uint8_t idx = 0;
	// Reset string and integer data.
	for (idx=0 ; idx<NODE_STRING_DATA_INDEX_MAX ; idx++) _NODE_flush_string_data_value(idx);
	for (idx=0 ; idx<NODE_REGISTER_ADDRESS_MAX ; idx++) node_ctx.data.integer_data_value[idx] = 0;
}

/* FLUSH NODES LIST.
 * @param:	None.
 */
void _NODE_flush_list(void) {
	// Local variables.
	uint8_t idx = 0;
	// Reset node list.
	for (idx=0 ; idx<NODES_LIST_SIZE_MAX ; idx++) {
		NODES_LIST.list[idx].address = 0xFF;
		NODES_LIST.list[idx].board_id = DINFOX_BOARD_ID_ERROR;
	}
	NODES_LIST.count = 0;
}

/*** NODE functions ***/

/* INIT NODE LAYER.
 * @param:	None.
 * @return:	None.
 */
void NODE_init(void) {
	// Reset node list.
	_NODE_flush_list();
}

/* GET NODE BOARD NAME.
 * @param rs485_node:		Node to get name of.
 * @param board_name_ptr:	Pointer to string that will contain board name.
 * @return status:			Function execution status.
 */
NODE_status_t NODE_get_name(NODE_t* node, char_t** board_name_ptr) {
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
NODE_status_t NODE_get_last_string_data_index(NODE_t* node, uint8_t* last_string_data_index) {
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
NODE_status_t NODE_update_data(NODE_t* node, uint8_t string_data_index) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	NODE_single_data_ptr_t single_data_ptr;
	// Check board ID.
	_NODE_check_node_and_board_id();
	_NODE_check_function_pointer(update_data);
	// Flush line.
	_NODE_flush_string_data_value(string_data_index);
	// Update pointers.
	single_data_ptr.string_name_ptr = (char_t*) &(node_ctx.data.string_data_name[string_data_index]);
	single_data_ptr.string_value_ptr = (char_t*) &(node_ctx.data.string_data_value[string_data_index]);
	single_data_ptr.value_ptr = &(node_ctx.data.integer_data_value[string_data_index]);
	// Check node protocol.
	switch (NODES[node -> board_id].protocol) {
	case NODE_PROTOCOL_LBUS:
		// Check index to update common or specific data.
		if (string_data_index < DINFOX_STRING_DATA_INDEX_LAST) {
			status = DINFOX_update_data((node -> address), string_data_index, &single_data_ptr);
		}
		else {
			status = NODES[node -> board_id].functions.update_data((node -> address), string_data_index, &single_data_ptr);
		}
		break;
	case NODE_PROTOCOL_R4S8CR:
		status = NODES[node -> board_id].functions.update_data((node -> address), string_data_index, &single_data_ptr);
		break;
	default:
		status = NODE_ERROR_PROTOCOL;
		break;
	}
errors:
	return status;
}

/* PERFORM ALL NODE MEASUREMENTS.
 * @param node:		Node to update.
 * @return status:	Function execution status.
 */
NODE_status_t NODE_update_all_data(NODE_t* node) {
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
NODE_status_t NODE_read_string_data(NODE_t* node, uint8_t string_data_index, char_t** string_data_name_ptr, char_t** string_data_value_ptr) {
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
	(*string_data_name_ptr) = (char_t*) node_ctx.data.string_data_name[string_data_index];
	(*string_data_value_ptr) = (char_t*) node_ctx.data.string_data_value[string_data_index];
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
NODE_status_t NODE_write_register(NODE_t* node, uint8_t register_address, int32_t value, NODE_access_status_t* write_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	NODE_write_parameters_t write_input;
	// Check node and board ID.
	_NODE_check_node_and_board_id();
	_NODE_check_function_pointer(write_register);
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
	// Common write parameters.
#ifdef AM
	write_input.node_address = (node -> address);
#endif
	write_input.value = value;
	write_input.register_address = register_address;
	// Check node protocol.
	switch (NODES[node -> board_id].protocol) {
	case NODE_PROTOCOL_LBUS:
		// Specific write parameters.
		write_input.timeout_ms = LBUS_TIMEOUT_MS;
		write_input.format = (register_address < DINFOX_REGISTER_LAST) ? DINFOX_REGISTERS_FORMAT[register_address] : NODES[node -> board_id].registers_format[register_address - DINFOX_REGISTER_LAST];
		break;
	case NODE_PROTOCOL_R4S8CR:
		// Specific write parameters.
		write_input.timeout_ms = R4S8CR_TIMEOUT_MS;
		write_input.format = NODES[node -> board_id].registers_format[register_address];
		break;
	default:
		status = NODE_ERROR_PROTOCOL;
		break;
	}
	status = NODES[node -> board_id].functions.write_register(&write_input, write_status);
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
NODE_status_t NODE_write_string_data(NODE_t* node, uint8_t string_data_index, int32_t value, NODE_access_status_t* write_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	uint8_t register_address = string_data_index;
	// Convert string data index to register.
	if ((NODES[node ->board_id].protocol == NODE_PROTOCOL_LBUS) && (string_data_index >= DINFOX_STRING_DATA_INDEX_LAST)) {
		register_address = (string_data_index + DINFOX_REGISTER_LAST - DINFOX_STRING_DATA_INDEX_LAST);
	}
	// Write register.
	status = NODE_write_register(node, register_address, value, write_status);
	return status;
}

/* SCAN ALL NODE ON BUS.
 * @param:			None.
 * @return status:	Function executions status.
 */
NODE_status_t NODE_scan(void) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	uint8_t lbus_nodes_count = 0;
	// Reset list.
	_NODE_flush_list();
	// Add master board to the list.
	NODES_LIST.list[0].board_id = DINFOX_BOARD_ID_DMM;
	NODES_LIST.list[0].address = DINFOX_RS485_ADDRESS_DMM;
	NODES_LIST.count++;
	// Scan LBUS nodes.
	status = LBUS_scan(&(NODES_LIST.list[NODES_LIST.count]), (NODES_LIST_SIZE_MAX - NODES_LIST.count), &lbus_nodes_count);
	if (status != NODE_SUCCESS) goto errors;
	// Update count.
	NODES_LIST.count += lbus_nodes_count;
	// Scan R4S8CR nodes.
	status = R4S8CR_scan(&(NODES_LIST.list[NODES_LIST.count]), (NODES_LIST_SIZE_MAX - NODES_LIST.count), &lbus_nodes_count);
	if (status != NODE_SUCCESS) goto errors;
	// Update count.
	NODES_LIST.count += lbus_nodes_count;
errors:
	return status;
}

/* GET NODE SIGFOX PAYLOAD.
 * @param node:				Node to read.
 * @param ul_payload:		Pointer that will contain UL payload of the node.
 * @param ul_payload_size:	Pointer to byte that will contain UL payload size.
 * @return status:			Function execution status.
 */
NODE_status_t NODE_get_sigfox_payload(NODE_t* node, NODE_sigfox_payload_type_t sigfox_payload_type, uint8_t* ul_payload, uint8_t* ul_payload_size) {
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
