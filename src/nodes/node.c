/*
 * node.c
 *
 *  Created on: 23 jan. 2023
 *      Author: Ludo
 */

#include "node.h"

#include "at_bus.h"
#include "bpsm.h"
#include "ddrm.h"
#include "dinfox.h"
#include "lpuart.h"
#include "lvrm.h"
#include "r4s8cr.h"
#include "rtc.h"
#include "uhfm.h"

/*** NODE local macros ***/

#define NODE_STRING_DATA_INDEX_MAX			32
#define NODE_REGISTER_ADDRESS_MAX			64

#define NODE_SIGFOX_PAYLOAD_STARTUP_SIZE	8
#define NODE_SIGFOX_PAYLOAD_SIZE_MAX		12

#define NODE_SIGFOX_UL_PERIOD_SECONDS		600
#define NODE_SIGFOX_DL_PERIOD_SECONDS		21600
#define NODE_SIGFOX_LOOP_MAX				100

#define NODE_ACTIONS_DEPTH					10

/*** NODE local structures ***/

typedef enum {
	NODE_PROTOCOL_AT_BUS = 0,
	NODE_PROTOCOL_R4S8CR,
	NODE_PROTOCOL_LAST
} NODE_protocol_t;

typedef enum {
	NODE_DOWNLINK_OPERATION_CODE_SINGLE_WRITE = 0,
	NODE_DOWNLINK_OPERATION_CODE_TOGGLE_OFF_ON,
	NODE_DOWNLINK_OPERATION_CODE_TOGGLE_ON_OFF,
	NODE_DOWNLINK_OPERATION_CODE_LAST
} NODE_downlink_operation_code_t;

typedef NODE_status_t (*NODE_read_register_t)(NODE_read_parameters_t* read_params, NODE_read_data_t* read_data, NODE_access_status_t* read_status);
typedef NODE_status_t (*NODE_write_register_t)(NODE_write_parameters_t* write_params, NODE_access_status_t* write_status);
typedef NODE_status_t (*NODE_update_data_t)(NODE_data_update_t* data_update);
typedef NODE_status_t (*NODE_get_sigfox_payload_t)(int32_t* integer_data_value, NODE_sigfox_ul_payload_type_t ul_payload_type, uint8_t* ul_payload, uint8_t* ul_payload_size);

typedef struct {
	NODE_read_register_t read_register;
	NODE_write_register_t write_register;
	NODE_update_data_t update_data;
	NODE_get_sigfox_payload_t get_sigfox_ul_payload;
} NODE_functions_t;

typedef struct {
	char_t* name;
	NODE_protocol_t protocol;
	uint8_t last_register_address;
	uint8_t last_string_data_index;
	STRING_format_t* registers_format;
	NODE_functions_t functions;
} NODE_descriptor_t;

typedef union {
	uint8_t frame[UHFM_SIGFOX_UL_PAYLOAD_SIZE_MAX];
	struct {
		unsigned node_address : 8;
		unsigned board_id : 8;
		uint8_t node_data[NODE_SIGFOX_PAYLOAD_SIZE_MAX - 2];
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} NODE_sigfox_ul_payload_t;

typedef union {
	uint8_t frame[NODE_SIGFOX_PAYLOAD_STARTUP_SIZE];
	struct {
		unsigned reset_reason : 8;
		unsigned major_version : 8;
		unsigned minor_version : 8;
		unsigned commit_index : 8;
		unsigned commit_id : 28;
		unsigned dirty_flag : 4;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} NODE_sigfox_payload_startup_t;

typedef union {
	uint8_t frame[UHFM_SIGFOX_DL_PAYLOAD_SIZE];
	struct {
		unsigned node_address : 8;
		unsigned board_id : 8;
		unsigned register_address : 8;
		unsigned operation_code : 8;
		unsigned data : 32;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} NODE_sigfox_dl_payload_t;

typedef struct {
	NODE_t* node;
	uint8_t register_address;
	int32_t register_value;
	uint32_t timestamp_seconds;
} NODE_action_t;

typedef struct {
	char_t string_data_name[NODE_STRING_DATA_INDEX_MAX][NODE_STRING_BUFFER_SIZE];
	char_t string_data_value[NODE_STRING_DATA_INDEX_MAX][NODE_STRING_BUFFER_SIZE];
	int32_t registers_value[NODE_REGISTER_ADDRESS_MAX];
} NODE_data_t;

typedef struct {
	NODE_data_t data;
	NODE_address_t uhfm_address;
	// Uplink.
	NODE_sigfox_ul_payload_t sigfox_ul_payload;
	NODE_sigfox_ul_payload_type_t sigfox_ul_payload_type_index;
	uint8_t sigfox_ul_payload_size;
	uint32_t sigfox_ul_next_time_seconds;
	uint8_t sigfox_ul_node_list_index;
	// Downlink.
	NODE_sigfox_dl_payload_t sigfox_dl_payload;
	uint32_t sigfox_dl_next_time_seconds;
	// Write actions list.
	NODE_action_t actions[NODE_ACTIONS_DEPTH];
	uint8_t actions_index;
} NODE_context_t;

/*** NODE local global variables ***/

// Note: table is indexed with board ID.
static const NODE_descriptor_t NODES[DINFOX_BOARD_ID_LAST] = {
	{"LVRM", NODE_PROTOCOL_AT_BUS, LVRM_REGISTER_LAST, LVRM_STRING_DATA_INDEX_LAST, (STRING_format_t*) LVRM_REGISTERS_FORMAT,
		{&AT_BUS_read_register, &AT_BUS_write_register, &LVRM_update_data, &LVRM_get_sigfox_ul_payload}
	},
	{"BPSM", NODE_PROTOCOL_AT_BUS, BPSM_REGISTER_LAST, BPSM_STRING_DATA_INDEX_LAST, (STRING_format_t*) BPSM_REGISTERS_FORMAT,
		{&AT_BUS_read_register, &AT_BUS_write_register, &BPSM_update_data, &BPSM_get_sigfox_ul_payload}
	},
	{"DDRM", NODE_PROTOCOL_AT_BUS, DDRM_REGISTER_LAST, DDRM_STRING_DATA_INDEX_LAST, (STRING_format_t*) DDRM_REGISTERS_FORMAT,
		{&AT_BUS_read_register, &AT_BUS_write_register, &DDRM_update_data, &DDRM_get_sigfox_ul_payload}
	},
	{"UHFM", NODE_PROTOCOL_AT_BUS, UHFM_REGISTER_LAST, UHFM_STRING_DATA_INDEX_LAST, (STRING_format_t*) UHFM_REGISTERS_FORMAT,
		{&AT_BUS_read_register, &AT_BUS_write_register, &UHFM_update_data, &UHFM_get_sigfox_ul_payload}
	},
	{"GPSM", NODE_PROTOCOL_AT_BUS, 0, 0, NULL,
		{&AT_BUS_read_register, &AT_BUS_write_register, NULL, NULL}
	},
	{"SM", NODE_PROTOCOL_AT_BUS, 0, 0, NULL,
		{&AT_BUS_read_register, &AT_BUS_write_register, NULL, NULL}
	},
	{"DIM", NODE_PROTOCOL_AT_BUS, 0, 0, NULL,
		{&AT_BUS_read_register, &AT_BUS_write_register, NULL, NULL}
	},
	{"RRM", NODE_PROTOCOL_AT_BUS, 0, 0, NULL,
		{&AT_BUS_read_register, &AT_BUS_write_register, NULL, NULL}
	},
	{"DMM", NODE_PROTOCOL_AT_BUS, 0, 0, NULL,
		{&AT_BUS_read_register, &AT_BUS_write_register, NULL, NULL}
	},
	{"MPMCM", NODE_PROTOCOL_AT_BUS, 0, 0, NULL,
		{&AT_BUS_read_register, &AT_BUS_write_register, NULL, NULL}
	},
	{"R4S8CR", NODE_PROTOCOL_R4S8CR, R4S8CR_REGISTER_LAST, R4S8CR_STRING_DATA_INDEX_LAST, (STRING_format_t*) R4S8CR_REGISTERS_FORMAT,
		{&R4S8CR_read_register, &R4S8CR_write_register, &R4S8CR_update_data, &R4S8CR_get_sigfox_ul_payload}},
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
	for (idx=0 ; idx<NODE_REGISTER_ADDRESS_MAX ; idx++) node_ctx.data.registers_value[idx] = 0;
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

/* WRITE NODE DATA.
 * @param node:				Node to write.
 * @param register_address:	Register address/
 * @param value:			Value to write in corresponding register.
 * @param write_status:		Pointer to the writing operation status.
 * @return status:			Function execution status.
 */
NODE_status_t _NODE_write_register(NODE_t* node, uint8_t register_address, int32_t value, NODE_access_status_t* write_status) {
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
	write_input.node_address = (node -> address);
	write_input.value = value;
	write_input.register_address = register_address;
	// Check node protocol.
	switch (NODES[node -> board_id].protocol) {
	case NODE_PROTOCOL_AT_BUS:
		// Specific write parameters.
		write_input.timeout_ms = AT_BUS_DEFAULT_TIMEOUT_MS;
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

/* SEND NODE DATA THROUGH RADIO.
 * @param node:					Node to monitor by radio.
 * @param sigfox_payload_type:	Type of data to send.
 * @param bidirectional_flag:	Downlink request flag.
 * @return status:				Function execution status.
 */
NODE_status_t _NODE_radio_send(NODE_t* node, NODE_sigfox_ul_payload_type_t ul_payload_type, uint8_t bidirectional_flag) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	uint8_t sigfox_payload_specific_size;
	NODE_sigfox_payload_startup_t sigfox_payload_startup;
	UHFM_sigfox_message_t sigfox_message;
	NODE_access_status_t send_status;
	uint8_t idx = 0;
	// Check board ID.
	_NODE_check_node_and_board_id();
	_NODE_check_function_pointer(get_sigfox_ul_payload);
	// Reset payload.
	for (idx=0 ; idx<NODE_SIGFOX_PAYLOAD_SIZE_MAX ; idx++) node_ctx.sigfox_ul_payload.frame[idx] = 0x00;
	node_ctx.sigfox_ul_payload_size = 0;
	// Add board ID and node address.
	node_ctx.sigfox_ul_payload.board_id = (node -> board_id);
	node_ctx.sigfox_ul_payload.node_address = (node -> address);
	node_ctx.sigfox_ul_payload_size = 2;
	// Add specific payload.
	switch (ul_payload_type) {
	case NODE_SIGFOX_PAYLOAD_TYPE_STARTUP:
		// Check node protocol.
		if (NODES[node -> board_id].protocol != NODE_PROTOCOL_AT_BUS) {
			status = NODE_ERROR_SIGFOX_PAYLOAD_EMPTY;
			goto errors;
		}
		// Build startup payload here since the format is common to all boards.
		sigfox_payload_startup.reset_reason = node_ctx.data.registers_value[DINFOX_REGISTER_RESET_REASON];
		sigfox_payload_startup.major_version = node_ctx.data.registers_value[DINFOX_REGISTER_SW_VERSION_MAJOR];
		sigfox_payload_startup.minor_version = node_ctx.data.registers_value[DINFOX_REGISTER_SW_VERSION_MINOR];
		sigfox_payload_startup.commit_index = node_ctx.data.registers_value[DINFOX_REGISTER_SW_VERSION_COMMIT_INDEX];
		sigfox_payload_startup.commit_id = node_ctx.data.registers_value[DINFOX_REGISTER_SW_VERSION_COMMIT_ID];
		sigfox_payload_startup.dirty_flag = node_ctx.data.registers_value[DINFOX_REGISTER_SW_VERSION_DIRTY_FLAG];
		// Add specific data to global paylaod.
		for (idx=0 ; idx<NODE_SIGFOX_PAYLOAD_STARTUP_SIZE ; idx++) {
			node_ctx.sigfox_ul_payload.node_data[idx] = sigfox_payload_startup.frame[idx];
		}
		node_ctx.sigfox_ul_payload_size += NODE_SIGFOX_PAYLOAD_STARTUP_SIZE;
		break;
	case NODE_SIGFOX_PAYLOAD_TYPE_MONITORING:
	case NODE_SIGFOX_PAYLOAD_TYPE_DATA:
		// Execute function of the corresponding board ID.
		status = NODES[node -> board_id].functions.get_sigfox_ul_payload(node_ctx.data.registers_value, ul_payload_type, node_ctx.sigfox_ul_payload.node_data, &sigfox_payload_specific_size);
		if (status != NODE_SUCCESS) goto errors;
		// Check returned pointer.
		if (sigfox_payload_specific_size == 0) {
			status = NODE_ERROR_SIGFOX_PAYLOAD_EMPTY;
			goto errors;
		}
		node_ctx.sigfox_ul_payload_size += sigfox_payload_specific_size;
		break;
	default:
		status = NODE_ERROR_SIGFOX_PAYLOAD_TYPE;
		goto errors;
	}
	// Check UHFM board availability.
	if (node_ctx.uhfm_address == DINFOX_NODE_ADDRESS_BROADCAST) {
		status = NODE_ERROR_NONE_RADIO_MODULE;
		goto errors;
	}
	// Build Sigfox message structure.
	sigfox_message.ul_payload = (uint8_t*) node_ctx.sigfox_ul_payload.frame;
	sigfox_message.ul_payload_size = node_ctx.sigfox_ul_payload_size;
	sigfox_message.bidirectional_flag = bidirectional_flag;
	sigfox_message.dl_payload = (uint8_t*) node_ctx.sigfox_dl_payload.frame;
	// Send message.
	status = UHFM_send_sigfox_message(node_ctx.uhfm_address, &sigfox_message, &send_status);
	if (status != NODE_SUCCESS) goto errors;
	// Check send status.
	if (send_status.all != 0) {
		status = NODE_ERROR_SIGFOX_SEND;
	}
errors:
	return status;
}

/* REMOVE ACTION IN LIST.
 * @param action_index:	Action to remove.
 * @return status:		Function execution status.
 */
	NODE_status_t _NODE_remove_action(uint8_t action_index) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	// Check parameter.
	if (action_index >= NODE_ACTIONS_DEPTH) {
		status = NODE_ERROR_ACTION_INDEX;
		goto errors;
	}
	node_ctx.actions[action_index].node = NULL;
	node_ctx.actions[action_index].register_address = 0x00;
	node_ctx.actions[action_index].register_value = 0;
	node_ctx.actions[action_index].timestamp_seconds = 0;
errors:
	return status;
}

/* RECORD NEW ACTION IN LIST.
 * @param action:	Pointer to the action to store.
 * @return status:	Function execution status.
 */
NODE_status_t _NODE_record_action(NODE_action_t* action) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	// Check parameter.
	if (action == NULL) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Store action.
	node_ctx.actions[node_ctx.actions_index].node = (action -> node);
	node_ctx.actions[node_ctx.actions_index].register_address = (action -> register_address);
	node_ctx.actions[node_ctx.actions_index].register_value = (action -> register_value);
	node_ctx.actions[node_ctx.actions_index].timestamp_seconds = (action -> timestamp_seconds);
	// Increment index.
	node_ctx.actions_index = (node_ctx.actions_index + 1) % NODE_ACTIONS_DEPTH;
errors:
	return status;
}

/* PARSE SIGFOX DL PAYLOAD.
 * @param:			None.
 * @return status:	Function execution status..
 */
NODE_status_t _NODE_execute_downlink(void) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	NODE_action_t action;
	uint8_t address_match = 0;
	uint8_t idx = 0;
	// Search board in nodes list.
	for (idx=0 ; idx<NODES_LIST.count ; idx++) {
		// Compare address
		if (NODES_LIST.list[idx].address == node_ctx.sigfox_dl_payload.node_address) {
			address_match = 1;
			break;
		}
	}
	// Check flag.
	if (address_match == 0) {
		status = NODE_ERROR_DOWNLINK_NODE_ADDRESS;
		goto errors;
	}
	// Check board ID.
	if (NODES_LIST.list[idx].board_id != node_ctx.sigfox_dl_payload.board_id) {
		status = NODE_ERROR_DOWNLINK_BOARD_ID;
		goto errors;
	}
	// Create action structure.
	action.node = &NODES_LIST.list[idx];
	action.register_address = node_ctx.sigfox_dl_payload.register_address;
	// Check operation code.
	switch (node_ctx.sigfox_dl_payload.operation_code) {
	case NODE_DOWNLINK_OPERATION_CODE_SINGLE_WRITE:
		// Instantaneous write operation.
		action.register_value = node_ctx.sigfox_dl_payload.data;
		action.timestamp_seconds = 0;
		_NODE_record_action(&action);
		break;
	case NODE_DOWNLINK_OPERATION_CODE_TOGGLE_OFF_ON:
		// Instantaneous OFF command.
		action.register_value = 0;
		action.timestamp_seconds = 0;
		_NODE_record_action(&action);
		// Program ON command.
		action.register_value = 1;
		action.timestamp_seconds = RTC_get_time_seconds() + node_ctx.sigfox_dl_payload.data;
		_NODE_record_action(&action);
		break;
	case NODE_DOWNLINK_OPERATION_CODE_TOGGLE_ON_OFF:
		// Instantaneous ON command.
		action.register_value = 1;
		action.timestamp_seconds = 0;
		_NODE_record_action(&action);
		// Program OFF command.
		action.register_value = 0;
		action.timestamp_seconds = RTC_get_time_seconds() + node_ctx.sigfox_dl_payload.data;
		_NODE_record_action(&action);
		break;
	default:
		status = NODE_ERROR_DOWNLINK_OPERATION_CODE;
		break;
	}
errors:
	return status;
}

/* CHECK AND EXECUTE NODE ACTIONS.
 * @param:			None.
 * @return status:	Function execution status..
 */
NODE_status_t _NODE_execute_actions(void) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	NODE_access_status_t write_status;
	uint8_t idx = 0;
	// Loop on action table.
	for (idx=0 ; idx<NODE_ACTIONS_DEPTH ; idx++) {
		// Check NODE pointer and timestamp.
		if ((node_ctx.actions[idx].node != NULL) && (RTC_get_time_seconds() >= node_ctx.actions[idx].timestamp_seconds)) {
			// Perform write operation.
			status = _NODE_write_register(node_ctx.actions[idx].node, node_ctx.actions[idx].register_address, node_ctx.actions[idx].register_value, &write_status);
			if (status != NODE_SUCCESS) goto errors;
			// Remove action.
			status = _NODE_remove_action(idx);
			if (status != NODE_SUCCESS) goto errors;
		}
	}
errors:
	return status;
}

/*** NODE functions ***/

/* INIT NODE LAYER.
 * @param:	None.
 * @return:	None.
 */
void NODE_init(void) {
	// Local variables.
	uint8_t idx = 0;
	// Reset node list.
	_NODE_flush_list();
	// Init context.
	node_ctx.sigfox_ul_node_list_index = 0;
	node_ctx.sigfox_ul_next_time_seconds = 0;
	node_ctx.sigfox_ul_payload_type_index = 0;
	node_ctx.sigfox_dl_next_time_seconds = 0;
	for (idx=0 ; idx<NODE_ACTIONS_DEPTH ; idx++) _NODE_remove_action(idx);
	node_ctx.actions_index = 0;
	// Init interface layers.
	AT_BUS_init();
}

/* GET NODE BOARD NAME.
 * @param node:				Node to get name of.
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
 * @param node:						Node to get name of.
 * @param last_string_data_index:	Pointer to byte that will contain last string index.
 * @return status:					Function execution status.
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
	NODE_data_update_t data_update;
	// Check board ID.
	_NODE_check_node_and_board_id();
	_NODE_check_function_pointer(update_data);
	// Flush line.
	_NODE_flush_string_data_value(string_data_index);
	// Update pointers.
	data_update.node_address = (node -> address);
	data_update.string_data_index = string_data_index;
	data_update.name_ptr = (char_t*) &(node_ctx.data.string_data_name[string_data_index]);
	data_update.value_ptr = (char_t*) &(node_ctx.data.string_data_value[string_data_index]);
	data_update.registers_value_ptr = (int32_t*) node_ctx.data.registers_value;
	// Check node protocol.
	switch (NODES[node -> board_id].protocol) {
	case NODE_PROTOCOL_AT_BUS:
		// Check index to update common or specific data.
		if (string_data_index < DINFOX_STRING_DATA_INDEX_LAST) {
			status = DINFOX_update_data(&data_update);
		}
		else {
			status = NODES[node -> board_id].functions.update_data(&data_update);
		}
		break;
	case NODE_PROTOCOL_R4S8CR:
		status = NODES[node -> board_id].functions.update_data(&data_update);
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
	// Check indexes.
	if ((NODES[node -> board_id].last_string_data_index) == 0) {
		status = NODE_ERROR_NOT_SUPPORTED;
		goto errors;
	}
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
NODE_status_t NODE_write_string_data(NODE_t* node, uint8_t string_data_index, int32_t value, NODE_access_status_t* write_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	uint8_t register_address = string_data_index;
	// Convert string data index to register.
	if ((NODES[node ->board_id].protocol == NODE_PROTOCOL_AT_BUS) && (string_data_index >= DINFOX_STRING_DATA_INDEX_LAST)) {
		register_address = (string_data_index + DINFOX_REGISTER_LAST - DINFOX_STRING_DATA_INDEX_LAST);
	}
	// Write register.
	status = _NODE_write_register(node, register_address, value, write_status);
	return status;
}

/* SCAN ALL NODE ON BUS.
 * @param:			None.
 * @return status:	Function executions status.
 */
NODE_status_t NODE_scan(void) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	uint8_t nodes_count = 0;
	uint8_t idx = 0;
	// Reset list.
	_NODE_flush_list();
	node_ctx.uhfm_address = DINFOX_NODE_ADDRESS_BROADCAST;
	// Add master board to the list.
	NODES_LIST.list[0].board_id = DINFOX_BOARD_ID_DMM;
	NODES_LIST.list[0].address = DINFOX_NODE_ADDRESS_DMM;
	NODES_LIST.count++;
	// Scan LBUS nodes.
	status = AT_BUS_scan(&(NODES_LIST.list[NODES_LIST.count]), (NODES_LIST_SIZE_MAX - NODES_LIST.count), &nodes_count);
	if (status != NODE_SUCCESS) goto errors;
	// Update count.
	NODES_LIST.count += nodes_count;
	// Search UHFM board in nodes list.
	for (idx=0 ; idx<NODES_LIST.count ; idx++) {
		// Check board ID.
		if (NODES_LIST.list[idx].board_id == DINFOX_BOARD_ID_UHFM) {
			node_ctx.uhfm_address = NODES_LIST.list[idx].address;
			break;
		}
	}
	// Scan R4S8CR nodes.
	status = R4S8CR_scan(&(NODES_LIST.list[NODES_LIST.count]), (NODES_LIST_SIZE_MAX - NODES_LIST.count), &nodes_count);
	if (status != NODE_SUCCESS) goto errors;
	// Update count.
	NODES_LIST.count += nodes_count;
errors:
	return status;
}

/* MAIN TASK OF NODE LAYER.
 * @param:			None.
 * @return status:	Function execution status.
 */
NODE_status_t NODE_task(void) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	LPUART_status_t lpuart1_status = LPUART_SUCCESS;
	uint32_t loop_count = 0;
	uint8_t bidirectional_flag = 0;
	uint8_t node_update_required = 1;
	// Turn bus interface on.
	lpuart1_status = LPUART1_power_on();
	LPUART1_status_check(NODE_ERROR_BASE_LPUART);
	// Check uplink period.
	if (RTC_get_time_seconds() >= node_ctx.sigfox_ul_next_time_seconds) {
		// Update next time.
		node_ctx.sigfox_ul_next_time_seconds += NODE_SIGFOX_UL_PERIOD_SECONDS;
		// Check downlink period.
		if (RTC_get_time_seconds() >= node_ctx.sigfox_dl_next_time_seconds) {
			// Update next time and set bidirectional flag.
			node_ctx.sigfox_dl_next_time_seconds += NODE_SIGFOX_DL_PERIOD_SECONDS;
			bidirectional_flag = 1;
		}
		// Search next Sigfox message to send.
		do {
			// Update node data if needed.
			if (node_update_required != 0) {
				status = NODE_update_all_data(&(NODES_LIST.list[node_ctx.sigfox_ul_node_list_index]));
				node_update_required = 0;
			}
			else {
				status = NODE_SUCCESS;
			}
			if (status == NODE_SUCCESS) {
				// Send data through radio.
				status = _NODE_radio_send(&(NODES_LIST.list[node_ctx.sigfox_ul_node_list_index]), node_ctx.sigfox_ul_payload_type_index, bidirectional_flag);
				// Handle all errors except not supported and empty payload.
				if ((status != NODE_SUCCESS) && (status != NODE_ERROR_NOT_SUPPORTED) && (status != NODE_ERROR_SIGFOX_PAYLOAD_EMPTY)) goto errors;
			}
			else {
				// Handle all errors except not supported.
				if (status != NODE_ERROR_NOT_SUPPORTED) goto errors;
			}
			// Increment payload type index.
			node_ctx.sigfox_ul_payload_type_index++;
			if (node_ctx.sigfox_ul_payload_type_index >= NODE_SIGFOX_PAYLOAD_TYPE_LAST) {
				// Switch to next node.
				node_ctx.sigfox_ul_payload_type_index = 0;
				node_ctx.sigfox_ul_node_list_index++;
				node_update_required = 1;
				if (node_ctx.sigfox_ul_node_list_index >= NODES_LIST.count) {
					// Come back to first node.
					node_ctx.sigfox_ul_node_list_index = 0;
				}
			}
			// Exit if timeout.
			loop_count++;
			if (loop_count > NODE_SIGFOX_LOOP_MAX) {
				status = NODE_ERROR_SIGFOX_LOOP;
				goto errors;
			}

		}
		while (status != NODE_SUCCESS);
	}
	// Execute downlink operation if needed.
	if (bidirectional_flag != 0) {
		status = _NODE_execute_downlink();
		if (status != NODE_SUCCESS) goto errors;
	}
	// Execute node actions.
	status = _NODE_execute_actions();
errors:
	// Turn bus interface off.
	LPUART1_power_off();
	return status;
}
