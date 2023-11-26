/*
 * r4s8cr.c
 *
 *  Created on: Feb 5, 2023
 *      Author: ludo
 */

#include "r4s8cr.h"

#include "dinfox.h"
#include "error.h"
#include "lpuart.h"
#include "node.h"
#include "node_common.h"
#include "r4s8cr_reg.h"
#include "xm.h"

/*** R4S8CR local macros ***/

#define R4S8CR_NUMBER_OF_RELAYS						8
#define R4S8CR_NUMBER_OF_IDS						15

#define R4S8CR_NODE_ADDRESS							0xFF

#define R4S8CR_BAUD_RATE							9600

#define R4S8CR_BUFFER_SIZE_BYTES					64
#define R4S8CR_REPLY_BUFFER_DEPTH					16

#define R4S8CR_ADDRESS_SIZE_BYTES					1
#define R4S8CR_RELAY_ADDRESS_SIZE_BYTES				1
#define R4S8CR_COMMAND_SIZE_BYTES					1

#define R4S8CR_COMMAND_READ							0xA0
#define R4S8CR_COMMAND_OFF							0x00
#define R4S8CR_COMMAND_ON							0x01

#define R4S8CR_REPLY_PARSING_DELAY_MS				10

#define R4S8CR_REPLY_HEADER_SIZE					(R4S8CR_ADDRESS_SIZE_BYTES + R4S8CR_RELAY_ADDRESS_SIZE_BYTES)
#define R4S8CR_REPLY_SIZE_BYTES						(R4S8CR_REPLY_HEADER_SIZE + R4S8CR_NUMBER_OF_RELAYS)

#define R4S8CR_SIGFOX_UL_PAYLOAD_ELECTRICAL_SIZE	2

/*** R4S8CR local structures ***/

/*******************************************************************/
typedef struct {
	uint8_t command[R4S8CR_BUFFER_SIZE_BYTES];
	uint8_t command_size;
	volatile uint8_t reply[R4S8CR_BUFFER_SIZE_BYTES];
	volatile uint8_t reply_size;
	DINFOX_bit_representation_t rxst[R4S8CR_NUMBER_OF_RELAYS];
} R4S8CR_context_t;

/*******************************************************************/
typedef union {
	uint8_t frame[R4S8CR_SIGFOX_UL_PAYLOAD_ELECTRICAL_SIZE];
	struct {
		unsigned r8stst : 2;
		unsigned r7stst : 2;
		unsigned r6stst : 2;
		unsigned r5stst : 2;
		unsigned r4stst : 2;
		unsigned r3stst : 2;
		unsigned r2stst : 2;
		unsigned r1stst : 2;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} R4S8CR_sigfox_ul_payload_data_t;

/*** R4S8CR local global variables ***/

static uint32_t R4S8CR_REGISTERS[R4S8CR_REG_ADDR_LAST];

static const uint32_t R4S8CR_REG_ERROR_VALUE[R4S8CR_REG_ADDR_LAST] = {
	((DINFOX_BIT_ERROR << 14) | (DINFOX_BIT_ERROR << 12) | (DINFOX_BIT_ERROR << 10) | (DINFOX_BIT_ERROR << 8) |
	 (DINFOX_BIT_ERROR << 6)  | (DINFOX_BIT_ERROR << 4)  | (DINFOX_BIT_ERROR << 2)  | (DINFOX_BIT_ERROR << 0)),
	0x00000000
};

static const XM_node_registers_t R4S8CR_NODE_REGISTERS = {
	.value = (uint32_t*) R4S8CR_REGISTERS,
	.error = (uint32_t*) R4S8CR_REG_ERROR_VALUE,
};

static const NODE_line_data_t R4S8CR_LINE_DATA[R4S8CR_LINE_DATA_INDEX_LAST] = {
	{"RELAY 1 =", STRING_NULL, STRING_FORMAT_DECIMAL, 0, R4S8CR_REG_ADDR_STATUS, R4S8CR_REG_STATUS_MASK_R1STST, R4S8CR_REG_ADDR_CONTROL, R4S8CR_REG_CONTROL_MASK_R1ST},
	{"RELAY 2 =", STRING_NULL, STRING_FORMAT_DECIMAL, 0, R4S8CR_REG_ADDR_STATUS, R4S8CR_REG_STATUS_MASK_R2STST, R4S8CR_REG_ADDR_CONTROL, R4S8CR_REG_CONTROL_MASK_R2ST},
	{"RELAY 3 =", STRING_NULL, STRING_FORMAT_DECIMAL, 0, R4S8CR_REG_ADDR_STATUS, R4S8CR_REG_STATUS_MASK_R3STST, R4S8CR_REG_ADDR_CONTROL, R4S8CR_REG_CONTROL_MASK_R3ST},
	{"RELAY 4 =", STRING_NULL, STRING_FORMAT_DECIMAL, 0, R4S8CR_REG_ADDR_STATUS, R4S8CR_REG_STATUS_MASK_R4STST, R4S8CR_REG_ADDR_CONTROL, R4S8CR_REG_CONTROL_MASK_R4ST},
	{"RELAY 5 =", STRING_NULL, STRING_FORMAT_DECIMAL, 0, R4S8CR_REG_ADDR_STATUS, R4S8CR_REG_STATUS_MASK_R5STST, R4S8CR_REG_ADDR_CONTROL, R4S8CR_REG_CONTROL_MASK_R5ST},
	{"RELAY 6 =", STRING_NULL, STRING_FORMAT_DECIMAL, 0, R4S8CR_REG_ADDR_STATUS, R4S8CR_REG_STATUS_MASK_R6STST, R4S8CR_REG_ADDR_CONTROL, R4S8CR_REG_CONTROL_MASK_R6ST},
	{"RELAY 7 =", STRING_NULL, STRING_FORMAT_DECIMAL, 0, R4S8CR_REG_ADDR_STATUS, R4S8CR_REG_STATUS_MASK_R7STST, R4S8CR_REG_ADDR_CONTROL, R4S8CR_REG_CONTROL_MASK_R7ST},
	{"RELAY 8 =", STRING_NULL, STRING_FORMAT_DECIMAL, 0, R4S8CR_REG_ADDR_STATUS, R4S8CR_REG_STATUS_MASK_R8STST, R4S8CR_REG_ADDR_CONTROL, R4S8CR_REG_CONTROL_MASK_R8ST},
};

static const uint8_t R4S8CR_REG_LIST_SIGFOX_UL_PAYLOAD_ELECTRICAL[] = {
	R4S8CR_REG_ADDR_STATUS,
};

static const DINFOX_register_access_t R4S8CR_REG_ACCESS[R4S8CR_REG_ADDR_LAST] = {
	DINFOX_REG_ACCESS_READ_ONLY,
	DINFOX_REG_ACCESS_READ_WRITE
};

static R4S8CR_context_t r4s8cr_ctx;

/*** R4S8CR local functions ***/

/*******************************************************************/
static void _R4S8CR_fill_rx_buffer(uint8_t rx_byte) {
	// Store incoming byte.
	r4s8cr_ctx.reply[r4s8cr_ctx.reply_size] = rx_byte;
	// Manage index.
	r4s8cr_ctx.reply_size = (r4s8cr_ctx.reply_size + 1) % R4S8CR_BUFFER_SIZE_BYTES;
}

/*******************************************************************/
static void _R4S8CR_flush_buffers(void) {
	// Local variables.
	uint8_t idx = 0;
	// Flush buffers.
	for (idx=0 ; idx<R4S8CR_BUFFER_SIZE_BYTES ; idx++) {
		r4s8cr_ctx.command[idx] = 0x00;
		r4s8cr_ctx.reply[idx] = 0x00;
	}
	r4s8cr_ctx.command_size = 0;
	r4s8cr_ctx.reply_size = 0;
}

/*******************************************************************/
static NODE_status_t _R4S8CR_configure_phy(void) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	LPUART_status_t lpuart1_status = LPUART_SUCCESS;
	LPUART_configuration_t lpuart_config;
	// Configure physical interface.
	lpuart_config.baud_rate = R4S8CR_BAUD_RATE;
	lpuart_config.rx_mode = LPUART_RX_MODE_DIRECT;
	lpuart_config.rx_callback = &_R4S8CR_fill_rx_buffer;
	lpuart1_status = LPUART1_configure(&lpuart_config);
	LPUART1_exit_error(NODE_ERROR_BASE_LPUART);
errors:
	return status;
}

/*******************************************************************/
static NODE_status_t _R4S8CR_write_relay_state(uint8_t relay_id, uint8_t rxst, NODE_access_status_t* write_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	LPUART_status_t lpuart1_status = LPUART_SUCCESS;
	// Check parameter.
	if ((relay_id == 0) || (relay_id > (R4S8CR_NUMBER_OF_RELAYS * R4S8CR_NUMBER_OF_IDS))) {
		status = NODE_ERROR_RELAY_ID;
		goto errors;
	}
	// No confirmation after write operation.
	(write_status -> all) = 0;
	// Flush buffers.
	_R4S8CR_flush_buffers();
	// Build command.
	r4s8cr_ctx.command[0] = R4S8CR_NODE_ADDRESS;
	r4s8cr_ctx.command[1] = relay_id;
	r4s8cr_ctx.command[2] = (rxst == 0) ? R4S8CR_COMMAND_OFF : R4S8CR_COMMAND_ON;
	r4s8cr_ctx.command_size = (R4S8CR_ADDRESS_SIZE_BYTES + R4S8CR_RELAY_ADDRESS_SIZE_BYTES + R4S8CR_COMMAND_SIZE_BYTES);
	// Configure physical interface.
	status = _R4S8CR_configure_phy();
	if (status != NODE_SUCCESS) goto errors;
	LPUART1_disable_rx();
	// Send command.
	lpuart1_status = LPUART1_write(r4s8cr_ctx.command, r4s8cr_ctx.command_size);
	LPUART1_exit_error(NODE_ERROR_BASE_LPUART);
	// Enable reception.
	LPUART1_enable_rx();
errors:
	return status;
}

/*******************************************************************/
static NODE_status_t _R4S8CR_read_relays_state(uint8_t relay_box_id, NODE_access_status_t* read_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	LPUART_status_t lpuart1_status = LPUART_SUCCESS;
	LPTIM_status_t lptim1_status = LPTIM_SUCCESS;
	uint32_t reply_time_ms = 0;
	uint8_t idx = 0;
	// Reset relays status.
	for (idx=0 ; idx<R4S8CR_NUMBER_OF_RELAYS ; idx++) {
		r4s8cr_ctx.rxst[idx] = DINFOX_BIT_ERROR;
	}
	// Flush buffers and status.
	_R4S8CR_flush_buffers();
	(read_status -> all) = 0;
	// Build command.
	r4s8cr_ctx.command[0] = R4S8CR_NODE_ADDRESS;
	r4s8cr_ctx.command[1] = (R4S8CR_COMMAND_READ | relay_box_id);
	r4s8cr_ctx.command[2] = 0x00;
	r4s8cr_ctx.command_size = (R4S8CR_ADDRESS_SIZE_BYTES + R4S8CR_RELAY_ADDRESS_SIZE_BYTES + R4S8CR_COMMAND_SIZE_BYTES);
	// Configure physical interface.
	status = _R4S8CR_configure_phy();
	if (status != NODE_SUCCESS) goto errors;
	LPUART1_disable_rx();
	// Send command.
	lpuart1_status = LPUART1_write(r4s8cr_ctx.command, r4s8cr_ctx.command_size);
	LPUART1_exit_error(NODE_ERROR_BASE_LPUART);
	// Enable reception.
	LPUART1_enable_rx();
	// Wait reply.
	while (1) {
		// Delay.
		lptim1_status = LPTIM1_delay_milliseconds(R4S8CR_REPLY_PARSING_DELAY_MS, LPTIM_DELAY_MODE_STOP);
		LPTIM1_exit_error(NODE_ERROR_BASE_LPTIM);
		reply_time_ms += R4S8CR_REPLY_PARSING_DELAY_MS;
		// Check number of received bytes.
		if (r4s8cr_ctx.reply_size >= R4S8CR_REPLY_SIZE_BYTES) {
			// Relays loop.
			for (idx=0 ; idx<R4S8CR_NUMBER_OF_RELAYS ; idx++) {
				// Get relay state.
				r4s8cr_ctx.rxst[idx] = (r4s8cr_ctx.reply[R4S8CR_REPLY_HEADER_SIZE + idx] == 0) ? DINFOX_BIT_0 : DINFOX_BIT_1;
			}
			break;
		}
		// Exit if timeout.
		if (reply_time_ms > R4S8CR_WRITE_TIMEOUT_MS) {
			// Set status to timeout.
			(read_status -> reply_timeout) = 1;
			break;
		}
	}
errors:
	LPUART1_disable_rx();
	return status;
}

/*******************************************************************/
static NODE_status_t _R4S8CR_update_register(NODE_address_t node_addr, uint8_t reg_addr, NODE_access_status_t* read_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	uint32_t unused_mask = 0;
	uint8_t relay_box_id = 0;
	uint8_t idx = 0;
	// Check parameters.
	if (read_status == NULL) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Check address.
	switch (reg_addr) {
	case R4S8CR_REG_ADDR_STATUS:
		// Convert node address to ID.
		relay_box_id = (node_addr - DINFOX_NODE_ADDRESS_R4S8CR_START + 1) & 0x0F;
		// Read relays state.
		status = _R4S8CR_read_relays_state(relay_box_id, read_status);
		if ((status != NODE_SUCCESS) || ((read_status -> all) != 0)) goto errors;
		// Write register.
		for (idx=0 ; idx<R4S8CR_NUMBER_OF_RELAYS ; idx++) {
			DINFOX_write_field(&(R4S8CR_REGISTERS[R4S8CR_REG_ADDR_STATUS]), &unused_mask, ((uint32_t) r4s8cr_ctx.rxst[idx]), (0b11 << (idx << 1)));
		}
		break;
	default:
		// Nothing to do on other registers.
		break;
	}
errors:
	return status;
}

/*******************************************************************/
static NODE_status_t _R4S8CR_check_register(NODE_address_t node_addr, uint8_t reg_addr, uint32_t reg_mask, NODE_access_status_t* write_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	uint32_t reg_value = 0;
	uint8_t relay_box_id = 0;
	uint8_t relay_id = 0;
	uint8_t idx = 0;
	// Check parameters.
	if (write_status == NULL) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Read register.
	reg_value = R4S8CR_REGISTERS[reg_addr];
	// Check address.
	switch (reg_addr) {
	case R4S8CR_REG_ADDR_CONTROL:
		// Convert node address to ID.
		relay_box_id = (node_addr - DINFOX_NODE_ADDRESS_R4S8CR_START + 1) & 0x0F;
		// Relay loop.
		for (idx=0 ; idx<R4S8CR_NUMBER_OF_RELAYS ; idx++) {
			// Compute relay id.
			relay_id = ((relay_box_id - 1) * 8) + idx + 1;
			// Check bit mask.
			if ((reg_mask & (0b1 << idx)) != 0) {
				// Set relay state
				status = _R4S8CR_write_relay_state(relay_id, (reg_value & (0b1 << idx)), write_status);
				if ((status != NODE_SUCCESS) || ((write_status -> all) != 0)) goto errors;
			}
		}
		break;
	default:
		// Nothing to do on other registers.
		break;
	}
errors:
	return status;
}

/*** R4S8CR functions ***/

/*******************************************************************/
void R4S8CR_init_registers(void) {
	// Local variables.
	uint8_t idx = 0;
	// Reset all registers.
	for (idx=0 ; idx<R4S8CR_REG_ADDR_LAST ; idx++) {
		R4S8CR_REGISTERS[idx] = R4S8CR_REG_ERROR_VALUE[idx];
	}
}

/*******************************************************************/
NODE_status_t R4S8CR_write_register(NODE_access_parameters_t* write_params, uint32_t reg_value, uint32_t reg_mask, NODE_access_status_t* write_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	NODE_status_t node_status = NODE_SUCCESS;
	uint32_t temp = 0;
	// Reset write status.
	(*write_status).all = 0;
	// Check parameters.
	if ((write_params == NULL) || (write_status == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	if ((write_params -> reg_addr) >= R4S8CR_REG_ADDR_LAST) {
		// Act as a slave.
		(*write_status).error_received = 1;
		goto errors;
	}
	if (R4S8CR_REG_ACCESS[(write_params -> reg_addr)] == DINFOX_REG_ACCESS_READ_ONLY) {
		// Act as a slave.
		(*write_status).error_received = 1;
		goto errors;
	}
	if (((write_params -> node_addr) < DINFOX_NODE_ADDRESS_R4S8CR_START) || ((write_params -> node_addr) >= (DINFOX_NODE_ADDRESS_R4S8CR_START + DINFOX_NODE_ADDRESS_RANGE_R4S8CR))) {
		// Act as a slave.
		(*write_status).reply_timeout = 1;
		goto errors;
	}
	// Read register.
	temp = R4S8CR_REGISTERS[(write_params -> reg_addr)];
	// Compute new value.
	temp &= ~reg_mask;
	temp |= (reg_value & reg_mask);
	// Write register.
	R4S8CR_REGISTERS[(write_params -> reg_addr)] = temp;
	// Check control bits.
	node_status = _R4S8CR_check_register((write_params -> node_addr), (write_params -> reg_addr), reg_mask, write_status);
	if (node_status != NODE_SUCCESS) {
		// Act as a slave.
		NODE_stack_error();
		(*write_status).error_received = 1;
		goto errors;
	}
errors:
	return status;
}

/*******************************************************************/
NODE_status_t R4S8CR_read_register(NODE_access_parameters_t* read_params, uint32_t* reg_value, NODE_access_status_t* read_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	NODE_status_t node_status = NODE_SUCCESS;
	// Check parameters.
	if ((read_params == NULL) || (reg_value == NULL) || (read_status == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	if ((read_params -> reg_addr) >= R4S8CR_REG_ADDR_LAST) {
		// Act as a slave.
		(*read_status).error_received = 1;
		goto errors;
	}
	if (((read_params -> node_addr) < DINFOX_NODE_ADDRESS_R4S8CR_START) || ((read_params -> node_addr) >= (DINFOX_NODE_ADDRESS_R4S8CR_START + DINFOX_NODE_ADDRESS_RANGE_R4S8CR))) {
		// Act as a slave.
		(*read_status).reply_timeout = 1;
		goto errors;
	}
	// Reset value.
	(*reg_value) = R4S8CR_REG_ERROR_VALUE[(read_params -> reg_addr)];
	// Update register.
	node_status = _R4S8CR_update_register((read_params -> node_addr), (read_params -> reg_addr), read_status);
	if (node_status != NODE_SUCCESS) {
		// Act as a slave.
		NODE_stack_error();
		(*read_status).error_received = 1;
		goto errors;
	}
	// Read register.
	(*reg_value) = R4S8CR_REGISTERS[(read_params -> reg_addr)];
errors:
	return status;
}

/*******************************************************************/
NODE_status_t R4S8CR_scan(NODE_t* nodes_list, uint8_t nodes_list_size, uint8_t* nodes_count) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	NODE_access_parameters_t read_params;
	NODE_access_status_t read_status;
	NODE_address_t node_addr = 0;
	uint8_t node_list_idx = 0;
	uint32_t reg_value = 0;
	// Check parameters.
	if ((nodes_list == NULL) || (nodes_count == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Reset count.
	(*nodes_count) = 0;
	// Build read input common parameters.
	read_params.reg_addr = R4S8CR_REG_ADDR_STATUS;
	read_params.reply_params.type = NODE_REPLY_TYPE_VALUE;
	read_params.reply_params.timeout_ms = R4S8CR_READ_TIMEOUT_MS;
	// Loop on all addresses.
	for (node_addr=DINFOX_NODE_ADDRESS_R4S8CR_START ; node_addr<(DINFOX_NODE_ADDRESS_R4S8CR_START + DINFOX_NODE_ADDRESS_RANGE_R4S8CR) ; node_addr++) {
		// Update read parameters.
		read_params.node_addr = node_addr;
		// Ping address.
		status = R4S8CR_read_register(&read_params, &reg_value, &read_status);
		if (status != NODE_SUCCESS) goto errors;
		// Check reply status.
		if (read_status.all == 0) {
			// Node found.
			(*nodes_count)++;
			// Store address and reset board ID.
			nodes_list[node_list_idx].address = node_addr;
			nodes_list[node_list_idx].board_id = DINFOX_BOARD_ID_R4S8CR;
		}
	}
errors:
	return status;
}

/*******************************************************************/
NODE_status_t R4S8CR_write_line_data(NODE_line_data_write_t* line_data_write, NODE_access_status_t* write_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	NODE_access_parameters_t write_params;
	uint32_t reg_addr = 0;
	uint32_t reg_value = 0;
	uint32_t reg_mask = 0;
	uint32_t timeout_ms = 0;
	// Check parameters.
	if ((line_data_write == NULL) || (write_status == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Check index.
	if ((line_data_write -> line_data_index) >= R4S8CR_LINE_DATA_INDEX_LAST) {
		status = NODE_ERROR_LINE_DATA_INDEX;
		goto errors;
	}
	// Compute parameters.
	reg_addr = R4S8CR_LINE_DATA[(line_data_write -> line_data_index)].write_reg_addr;
	DINFOX_write_field(&reg_value, &reg_mask, (line_data_write -> field_value), R4S8CR_LINE_DATA[(line_data_write -> line_data_index)].write_field_mask);
	timeout_ms = R4S8CR_REG_WRITE_TIMEOUT_MS[reg_addr];
	// Write parameters.
	write_params.node_addr = (line_data_write -> node_addr);
	write_params.reg_addr = reg_addr;
	write_params.reply_params.type = NODE_REPLY_TYPE_NONE;
	write_params.reply_params.timeout_ms = timeout_ms;
	// Write register.
	status = R4S8CR_write_register(&write_params, reg_value, reg_mask, write_status);
errors:
	return status;
}

/*******************************************************************/
NODE_status_t R4S8CR_read_line_data(NODE_line_data_read_t* line_data_read, NODE_access_status_t* read_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	NODE_access_parameters_t read_params;
	STRING_status_t string_status = STRING_SUCCESS;
	uint32_t reg_value = 0;
	uint32_t field_value = 0;
	uint8_t str_data_idx = 0;
	uint8_t buffer_size = 0;
	// Check parameters.
	if (line_data_read == NULL) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	if (((line_data_read -> name_ptr) == NULL) || ((line_data_read -> value_ptr) == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Check index.
	if ((line_data_read -> line_data_index) >= R4S8CR_LINE_DATA_INDEX_LAST) {
		status = NODE_ERROR_LINE_DATA_INDEX;
		goto errors;
	}
	// Get string data index.
	str_data_idx = (line_data_read -> line_data_index);
	// Add data name.
	NODE_append_name_string((char_t*) R4S8CR_LINE_DATA[str_data_idx].name);
	buffer_size = 0;
	// Reset result to error.
	NODE_flush_string_value();
	NODE_append_value_string((char_t*) NODE_ERROR_STRING);
	// Read parameters.
	read_params.node_addr = (line_data_read -> node_addr);
	read_params.reg_addr = R4S8CR_LINE_DATA[str_data_idx].read_reg_addr;
	read_params.reply_params.type = NODE_REPLY_TYPE_VALUE;
	read_params.reply_params.timeout_ms = R4S8CR_READ_TIMEOUT_MS;
	// Update register.
	status = R4S8CR_read_register(&read_params, &reg_value, read_status);
	if ((status != NODE_SUCCESS) || ((read_status -> all) != 0)) goto errors;
	// Compute field.
	NODE_flush_string_value();
	field_value = DINFOX_read_field(reg_value, R4S8CR_LINE_DATA[str_data_idx].read_field_mask);
	// Check index.
	switch (line_data_read -> line_data_index) {
	case R4S8CR_LINE_DATA_INDEX_R1ST:
	case R4S8CR_LINE_DATA_INDEX_R2ST:
	case R4S8CR_LINE_DATA_INDEX_R3ST:
	case R4S8CR_LINE_DATA_INDEX_R4ST:
	case R4S8CR_LINE_DATA_INDEX_R5ST:
	case R4S8CR_LINE_DATA_INDEX_R6ST:
	case R4S8CR_LINE_DATA_INDEX_R7ST:
	case R4S8CR_LINE_DATA_INDEX_R8ST:
		// Specific print for boolean data.
		NODE_flush_string_value();
		switch (field_value) {
		case DINFOX_BIT_0:
			NODE_append_value_string("OFF");
			break;
		case DINFOX_BIT_1:
			NODE_append_value_string("ON");
			break;
		case DINFOX_BIT_FORCED_HARDWARE:
			NODE_append_value_string("HW");
			break;
		default:
			NODE_append_value_string("ERROR");
			break;
		}
		break;
	default:
		NODE_append_value_int32(field_value, STRING_FORMAT_HEXADECIMAL, 1);
		break;
	}
errors:
	return status;
}

/*******************************************************************/
NODE_status_t R4S8CR_build_sigfox_ul_payload(NODE_ul_payload_t* node_ul_payload) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	NODE_access_status_t access_status;
	XM_registers_list_t reg_list;
	R4S8CR_sigfox_ul_payload_data_t sigfox_ul_payload_data;
	uint8_t idx = 0;
	// Check parameters.
	if (node_ul_payload == NULL) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	if (((node_ul_payload -> node) == NULL) || ((node_ul_payload -> ul_payload) == NULL) || ((node_ul_payload -> size) == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Build registers list.
	reg_list.addr_list = (uint8_t*) R4S8CR_REG_LIST_SIGFOX_UL_PAYLOAD_ELECTRICAL;
	reg_list.size = sizeof(R4S8CR_REG_LIST_SIGFOX_UL_PAYLOAD_ELECTRICAL);
	// Read related registers.
	status = XM_read_registers((node_ul_payload -> node -> address), &reg_list, (XM_node_registers_t*) &R4S8CR_NODE_REGISTERS, &access_status);
	if (status != NODE_SUCCESS) goto errors;
	// Build data payload.
	sigfox_ul_payload_data.r1stst = DINFOX_read_field(R4S8CR_REGISTERS[R4S8CR_REG_ADDR_STATUS], R4S8CR_REG_STATUS_MASK_R1STST);
	sigfox_ul_payload_data.r2stst = DINFOX_read_field(R4S8CR_REGISTERS[R4S8CR_REG_ADDR_STATUS], R4S8CR_REG_STATUS_MASK_R2STST);
	sigfox_ul_payload_data.r3stst = DINFOX_read_field(R4S8CR_REGISTERS[R4S8CR_REG_ADDR_STATUS], R4S8CR_REG_STATUS_MASK_R3STST);
	sigfox_ul_payload_data.r4stst = DINFOX_read_field(R4S8CR_REGISTERS[R4S8CR_REG_ADDR_STATUS], R4S8CR_REG_STATUS_MASK_R4STST);
	sigfox_ul_payload_data.r5stst = DINFOX_read_field(R4S8CR_REGISTERS[R4S8CR_REG_ADDR_STATUS], R4S8CR_REG_STATUS_MASK_R5STST);
	sigfox_ul_payload_data.r6stst = DINFOX_read_field(R4S8CR_REGISTERS[R4S8CR_REG_ADDR_STATUS], R4S8CR_REG_STATUS_MASK_R6STST);
	sigfox_ul_payload_data.r7stst = DINFOX_read_field(R4S8CR_REGISTERS[R4S8CR_REG_ADDR_STATUS], R4S8CR_REG_STATUS_MASK_R7STST);
	sigfox_ul_payload_data.r8stst = DINFOX_read_field(R4S8CR_REGISTERS[R4S8CR_REG_ADDR_STATUS], R4S8CR_REG_STATUS_MASK_R8STST);
	// Copy payload.
	for (idx=0 ; idx<R4S8CR_SIGFOX_UL_PAYLOAD_ELECTRICAL_SIZE ; idx++) {
		(node_ul_payload -> ul_payload)[idx] = sigfox_ul_payload_data.frame[idx];
	}
	(*(node_ul_payload -> size)) = R4S8CR_SIGFOX_UL_PAYLOAD_ELECTRICAL_SIZE;
errors:
	return status;
}
