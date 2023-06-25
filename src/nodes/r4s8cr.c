/*
 * r4s8cr.c
 *
 *  Created on: Feb 5, 2023
 *      Author: ludo
 */

#include "r4s8cr.h"

#include "dinfox.h"
#include "lpuart.h"
#include "node.h"
#include "r4s8cr_reg.h"
#include "xm.h"

/*** R4S8CR local macros ***/

#define R4S8CR_NUMBER_OF_RELAYS					8
#define R4S8CR_NUMBER_OF_IDS					15

#define R4S8CR_NODE_ADDRESS						0xFF

#define R4S8CR_BAUD_RATE						9600

#define R4S8CR_BUFFER_SIZE_BYTES				64
#define R4S8CR_REPLY_BUFFER_DEPTH				16

#define R4S8CR_ADDRESS_SIZE_BYTES				1
#define R4S8CR_RELAY_ADDRESS_SIZE_BYTES			1
#define R4S8CR_COMMAND_SIZE_BYTES				1

#define R4S8CR_COMMAND_READ						0xA0
#define R4S8CR_COMMAND_OFF						0x00
#define R4S8CR_COMMAND_ON						0x01

#define R4S8CR_REPLY_PARSING_DELAY_MS			10

#define R4S8CR_REPLY_HEADER_SIZE				(R4S8CR_ADDRESS_SIZE_BYTES + R4S8CR_RELAY_ADDRESS_SIZE_BYTES)
#define R4S8CR_REPLY_SIZE_BYTES					(R4S8CR_REPLY_HEADER_SIZE + R4S8CR_NUMBER_OF_RELAYS)

#define R4S8CR_SIGFOX_PAYLOAD_ELECTRICAL_SIZE	1

/*** R4S8CR local structures ***/

typedef struct {
	uint8_t command[R4S8CR_BUFFER_SIZE_BYTES];
	uint8_t command_size;
	volatile uint8_t reply[R4S8CR_BUFFER_SIZE_BYTES];
	volatile uint8_t reply_size;
} R4S8CR_context_t;

typedef union {
	uint8_t frame[R4S8CR_SIGFOX_PAYLOAD_ELECTRICAL_SIZE];
	struct {
		unsigned r1st : 1;
		unsigned r2st : 1;
		unsigned r3st : 1;
		unsigned r4st : 1;
		unsigned r5st : 1;
		unsigned r6st : 1;
		unsigned r7st : 1;
		unsigned r8st : 1;
	} __attribute__((packed));
} R4S8CR_sigfox_payload_data_t;

/*** R4S8CR local global variables ***/

static uint32_t R4S8CR_REGISTERS[R4S8CR_REG_ADDR_LAST];

static const NODE_line_data_t R4S8CR_LINE_DATA[R4S8CR_LINE_DATA_INDEX_LAST] = {
	{"RELAY 1 =", STRING_NULL, STRING_FORMAT_BOOLEAN, 0, R4S8CR_REG_ADDR_STATUS_CONTROL, R4S8CR_REG_STATUS_CONTROL_MASK_R1ST},
	{"RELAY 2 =", STRING_NULL, STRING_FORMAT_BOOLEAN, 0, R4S8CR_REG_ADDR_STATUS_CONTROL, R4S8CR_REG_STATUS_CONTROL_MASK_R2ST},
	{"RELAY 3 =", STRING_NULL, STRING_FORMAT_BOOLEAN, 0, R4S8CR_REG_ADDR_STATUS_CONTROL, R4S8CR_REG_STATUS_CONTROL_MASK_R3ST},
	{"RELAY 4 =", STRING_NULL, STRING_FORMAT_BOOLEAN, 0, R4S8CR_REG_ADDR_STATUS_CONTROL, R4S8CR_REG_STATUS_CONTROL_MASK_R4ST},
	{"RELAY 5 =", STRING_NULL, STRING_FORMAT_BOOLEAN, 0, R4S8CR_REG_ADDR_STATUS_CONTROL, R4S8CR_REG_STATUS_CONTROL_MASK_R5ST},
	{"RELAY 6 =", STRING_NULL, STRING_FORMAT_BOOLEAN, 0, R4S8CR_REG_ADDR_STATUS_CONTROL, R4S8CR_REG_STATUS_CONTROL_MASK_R6ST},
	{"RELAY 7 =", STRING_NULL, STRING_FORMAT_BOOLEAN, 0, R4S8CR_REG_ADDR_STATUS_CONTROL, R4S8CR_REG_STATUS_CONTROL_MASK_R7ST},
	{"RELAY 8 =", STRING_NULL, STRING_FORMAT_BOOLEAN, 0, R4S8CR_REG_ADDR_STATUS_CONTROL, R4S8CR_REG_STATUS_CONTROL_MASK_R8ST},
};

static const uint32_t R4S8CR_REG_ERROR_VALUE[R4S8CR_REG_ADDR_LAST] = {
	0x00000000,
};

static const uint8_t R4S8CR_REG_LIST_SIGFOX_PAYLOAD_ELECTRICAL[] = {
	R4S8CR_REG_ADDR_STATUS_CONTROL,
};

static const DINFOX_register_access_t R4S8CR_REG_ACCESS[R4S8CR_REG_ADDR_LAST] = {
	DINFOX_REG_ACCESS_READ_WRITE
};

static R4S8CR_context_t r4s8cr_ctx;

/*** R4S8CR local functions ***/

/* FLUSH COMMAND BUFFER.
 * @param:	None.
 * @return:	None.
 */
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

/* CONFIGURE PHYSICAL INTERFACE.
 * @param:	None.
 * @return:	None.
 */
static NODE_status_t _R4S8CR_configure_phy(void) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	LPUART_status_t lpuart1_status = LPUART_SUCCESS;
	LPUART_config_t lpuart_config;
	// Configure physical interface.
	lpuart_config.baud_rate = R4S8CR_BAUD_RATE;
	lpuart_config.rx_mode = LPUART_RX_MODE_DIRECT;
	lpuart_config.rx_callback = &R4S8CR_fill_rx_buffer;
	lpuart1_status = LPUART1_configure(&lpuart_config);
	LPUART1_status_check(NODE_ERROR_BASE_LPUART);
errors:
	return status;
}

/* WRITE RELAY STATE.
 * @param relay_id:		Relay ID.
 * @param relay_state:	State to apply.
 * @return status:		Function execution status.
 */
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
	lpuart1_status = LPUART1_send(r4s8cr_ctx.command, r4s8cr_ctx.command_size);
	LPUART1_status_check(NODE_ERROR_BASE_LPUART);
	// Enable reception.
	LPUART1_enable_rx();
errors:
	return status;
}

/* READ RELAYS STATE.
 * @param relay_box_id:	Relay box ID.
 * @param timeout_ms:	Read operation timeout in ms.
 * @param rxst:			Pointer to the relays state.
 * @param read_status:	Pointer to the read operation status.
 * @return status:		Function execution status.
 */
static NODE_status_t _R4S8CR_read_relays_state(uint8_t relay_box_id, uint32_t timeout_ms, uint8_t* rxst, NODE_access_status_t* read_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	LPUART_status_t lpuart1_status = LPUART_SUCCESS;
	LPTIM_status_t lptim1_status = LPTIM_SUCCESS;
	uint32_t reply_time_ms = 0;
	uint8_t rst = 0;
	uint8_t idx = 0;
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
	lpuart1_status = LPUART1_send(r4s8cr_ctx.command, r4s8cr_ctx.command_size);
	LPUART1_status_check(NODE_ERROR_BASE_LPUART);
	// Enable reception.
	LPUART1_enable_rx();
	// Wait reply.
	while (1) {
		// Delay.
		lptim1_status = LPTIM1_delay_milliseconds(R4S8CR_REPLY_PARSING_DELAY_MS, LPTIM_DELAY_MODE_STOP);
		LPTIM1_status_check(NODE_ERROR_BASE_LPTIM);
		reply_time_ms += R4S8CR_REPLY_PARSING_DELAY_MS;
		// Check number of received bytes.
		if (r4s8cr_ctx.reply_size >= R4S8CR_REPLY_SIZE_BYTES) {
			// Update register.
			(*rxst) = 0;
			for (idx=0 ; idx<R4S8CR_NUMBER_OF_RELAYS ; idx++) {
				// Get relay state.
				rst = (r4s8cr_ctx.reply[R4S8CR_REPLY_HEADER_SIZE + idx] == 0) ? 0 : 1;
				// Update bit.
				(*rxst) |= (rst << idx);
			}
			break;
		}
		// Exit if timeout.
		if (reply_time_ms > timeout_ms) {
			// Set status to timeout.
			(read_status -> reply_timeout) = 1;
			break;
		}
	}
errors:
	LPUART1_disable_rx();
	return status;
}

/* UPDATE REGISTERS LIST.
 * @param node_addr:	Node address.
 * @param reg_list:		List of register to read.
 * @return status:		Function execution status.
 */
static NODE_status_t _R4S8CR_read_registers(NODE_address_t node_addr, XM_registers_list_t* reg_list) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	NODE_access_parameters_t read_params;
	NODE_access_status_t unused_read_status;
	uint8_t reg_addr = 0;
	uint8_t idx = 0;
	// Check parameters.
	if (reg_list == NULL) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	if ((reg_list -> addr_list) == NULL) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	if ((reg_list -> size) == 0) {
		status = NODE_ERROR_REGISTER_LIST_SIZE;
		goto errors;
	}
	// Build read parameters.
	read_params.node_addr = node_addr;
	read_params.reply_params.timeout_ms = R4S8CR_DEFAULT_TIMEOUT_MS;
	read_params.reply_params.type = NODE_REPLY_TYPE_VALUE;
	// Read all registers.
	for (idx=0 ; idx<(reg_list -> size) ; idx++) {
		// Read register.
		read_params.reg_addr = (reg_list -> addr_list)[idx];
		// Note: status is not checked is order fill all registers with their error value.
		R4S8CR_read_register(&read_params, &(R4S8CR_REGISTERS[reg_addr]), &unused_read_status);
	}
errors:
	return status;
}

/*** R4S8CR functions ***/

/* INIT R4S8CR REGISTERS.
 * @param:	None.
 * @return:	None.
 */
void R4S8CR_init_registers(void) {
	// Local variables.
	uint8_t idx = 0;
	// Reset all registers.
	for (idx=0 ; idx<R4S8CR_REG_ADDR_LAST ; idx++) {
		R4S8CR_REGISTERS[idx] = 0;
	}
}

/* WRITE R4S8CR NODE REGISTER.
 * @param write_params:	Pointer to the write operation parameters.
 * @param write_status:	Pointer to the write operation status.
 * @return status:		Function execution status.
 */
NODE_status_t R4S8CR_write_register(NODE_access_parameters_t* write_params, uint32_t reg_value, uint32_t reg_mask, NODE_access_status_t* write_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	uint8_t relay_box_id = 0;
	uint8_t relay_id = 0;
	uint32_t temp = 0;
	uint8_t idx = 0;
	// Reset write status.
	(*write_status).all = 0;
	// Check parameters.
	if ((write_params == NULL) || (write_status == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	if ((write_params -> reg_addr) >= R4S8CR_REG_ADDR_LAST) {
		status = NODE_ERROR_REGISTER_ADDRESS;
		goto errors;
	}
	if (((write_params -> node_addr) < DINFOX_NODE_ADDRESS_R4S8CR_START) || ((write_params -> node_addr) >= (DINFOX_NODE_ADDRESS_R4S8CR_START + DINFOX_NODE_ADDRESS_RANGE_R4S8CR))) {
		status = NODE_ERROR_NODE_ADDRESS;
		goto errors;
	}
	// Check access.
	if (R4S8CR_REG_ACCESS[(write_params -> reg_addr)] == DINFOX_REG_ACCESS_READ_ONLY) {
		status = NODE_ERROR_REGISTER_READ_ONLY;
		goto errors;
	}
	// Read register.
	temp = R4S8CR_REGISTERS[(write_params -> reg_addr)];
	// Compute new value.
	temp &= ~reg_mask;
	temp |= (reg_value & reg_mask);
	// Write register.
	R4S8CR_REGISTERS[(write_params -> reg_addr)] = temp;
	// Check actions.
	switch (write_params -> reg_addr) {
	case R4S8CR_REG_ADDR_STATUS_CONTROL:
		// Convert node address to ID.
		relay_box_id = ((write_params -> node_addr) - DINFOX_NODE_ADDRESS_R4S8CR_START + 1) & 0x0F;
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

/* READ R4S8CR NODE REGISTER.
 * @param read_params:	Pointer to the read operation parameters.
 * @param read_data:	Pointer to the read result.
 * @param read_status:	Pointer to the read operation status.
 * @return status:		Function execution status.
 */
NODE_status_t R4S8CR_read_register(NODE_access_parameters_t* read_params, uint32_t* reg_value, NODE_access_status_t* read_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	uint8_t relay_box_id = 0;
	uint8_t rxst = 0;
	// Check parameters.
	if ((read_params == NULL) || (reg_value == NULL) || (read_status == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	if ((read_params -> reg_addr) >= R4S8CR_REG_ADDR_LAST) {
		status = NODE_ERROR_REGISTER_ADDRESS;
		goto errors;
	}
	if (((read_params -> node_addr) < DINFOX_NODE_ADDRESS_R4S8CR_START) || ((read_params -> node_addr) >= (DINFOX_NODE_ADDRESS_R4S8CR_START + DINFOX_NODE_ADDRESS_RANGE_R4S8CR))) {
		status = NODE_ERROR_NODE_ADDRESS;
		goto errors;
	}
	// Reset value.
	(*reg_value) = R4S8CR_REG_ERROR_VALUE[(read_params -> reg_addr)];
	// Update required registers.
	switch (read_params -> reg_addr) {
	case R4S8CR_REG_ADDR_STATUS_CONTROL:
		// Convert node address to ID.
		relay_box_id = ((read_params -> node_addr) - DINFOX_NODE_ADDRESS_R4S8CR_START + 1) & 0x0F;
		// Read relays state.
		status = _R4S8CR_read_relays_state(relay_box_id, ((read_params -> reply_params).timeout_ms), &rxst, read_status);
		if ((status != NODE_SUCCESS) || ((read_status -> all) != 0)) goto errors;
		// Write register.
		DINFOX_write_field(&(R4S8CR_REGISTERS[R4S8CR_REG_ADDR_STATUS_CONTROL]), (uint32_t) (rxst), R4S8CR_REG_STATUS_CONTROL_MASK_ALL);
		break;
	default:
		// Nothing to do on other registers.
		break;
	}
	// Read register.
	(*reg_value) = R4S8CR_REGISTERS[(read_params -> reg_addr)];
errors:
	return status;
}

/* SCAN R4S8CR NODES ON BUS.
 * @param nodes_list:		Node list to fill.
 * @param nodes_list_size:	Maximum size of the list.
 * @param nodes_count:		Pointer to byte that will contain the number of LBUS nodes detected.
 * @return status:			Function execution status.
 */
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
	read_params.reg_addr = R4S8CR_REG_ADDR_STATUS_CONTROL;
	read_params.reply_params.type = NODE_REPLY_TYPE_VALUE;
	read_params.reply_params.timeout_ms = R4S8CR_DEFAULT_TIMEOUT_MS;
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

/* WRITE R4S8CR DATA.
 * @param line_data_write:	Pointer to the data write structure.
 * @param read_status:		Pointer to the writing operation status.
 * @return status:			Function execution status.
 */
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
	reg_addr = R4S8CR_LINE_DATA[(line_data_write -> line_data_index)].reg_addr;
	reg_mask = R4S8CR_LINE_DATA[(line_data_write -> line_data_index)].field_mask;
	reg_value |= (line_data_write -> field_value) << DINFOX_get_field_offset(reg_mask);
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

/* READ R4S8CR DATA.
 * @param line_data_read:	Pointer to the data read structure.
 * @param read_status:		Pointer to the reading operation status.
 * @return status:			Function execution status.
 */
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
	read_params.reg_addr = R4S8CR_LINE_DATA[str_data_idx].reg_addr;
	read_params.reply_params.type = NODE_REPLY_TYPE_VALUE;
	read_params.reply_params.timeout_ms = R4S8CR_DEFAULT_TIMEOUT_MS;
	// Update register.
	status = R4S8CR_read_register(&read_params, &reg_value, read_status);
	if ((status != NODE_SUCCESS) || ((read_status -> all) != 0)) goto errors;
	// Compute field.
	NODE_flush_string_value();
	field_value = DINFOX_read_field(reg_value, R4S8CR_LINE_DATA[str_data_idx].field_mask);
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
		NODE_append_value_string((field_value == 0) ? "OFF" : "ON");
		break;
	default:
		NODE_append_value_int32(field_value, STRING_FORMAT_HEXADECIMAL, 1);
		break;
	}
errors:
	return status;
}

/* UPDATE R4S8CR NODE SIGFOX UPLINK PAYLOAD.
 * @param ul_payload_update:	Pointer to the UL payload update structure.
 * @return status:				Function execution status.
 */
NODE_status_t R4S8CR_build_sigfox_ul_payload(NODE_ul_payload_update_t* ul_payload_update) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	XM_node_registers_t node_reg;
	XM_registers_list_t reg_list;
	R4S8CR_sigfox_payload_data_t sigfox_payload_data;
	uint32_t reg_value = 0;
	uint8_t idx = 0;
	// Check parameters.
	if (ul_payload_update == NULL) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	if (((ul_payload_update -> ul_payload) == NULL) || ((ul_payload_update -> size) == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Build node registers structure.
	node_reg.value = (uint32_t*) R4S8CR_REGISTERS;
	node_reg.error = (uint32_t*) R4S8CR_REG_ERROR_VALUE;
	// Build registers list.
	reg_list.addr_list = (uint8_t*) R4S8CR_REG_LIST_SIGFOX_PAYLOAD_ELECTRICAL;
	reg_list.size = sizeof(R4S8CR_REG_LIST_SIGFOX_PAYLOAD_ELECTRICAL);
	// Reset registers.
	status = XM_reset_registers(&reg_list, &node_reg);
	if (status != NODE_SUCCESS) goto errors;
	// Read related registers.
	status = _R4S8CR_read_registers((ul_payload_update -> node -> address), &reg_list);
	if (status != NODE_SUCCESS) goto errors;
	// Build data payload.
	sigfox_payload_data.r1st = DINFOX_read_field(reg_value, R4S8CR_REG_STATUS_CONTROL_MASK_R1ST);
	sigfox_payload_data.r2st = DINFOX_read_field(reg_value, R4S8CR_REG_STATUS_CONTROL_MASK_R2ST);
	sigfox_payload_data.r3st = DINFOX_read_field(reg_value, R4S8CR_REG_STATUS_CONTROL_MASK_R3ST);
	sigfox_payload_data.r4st = DINFOX_read_field(reg_value, R4S8CR_REG_STATUS_CONTROL_MASK_R4ST);
	sigfox_payload_data.r5st = DINFOX_read_field(reg_value, R4S8CR_REG_STATUS_CONTROL_MASK_R5ST);
	sigfox_payload_data.r6st = DINFOX_read_field(reg_value, R4S8CR_REG_STATUS_CONTROL_MASK_R6ST);
	sigfox_payload_data.r7st = DINFOX_read_field(reg_value, R4S8CR_REG_STATUS_CONTROL_MASK_R7ST);
	sigfox_payload_data.r8st = DINFOX_read_field(reg_value, R4S8CR_REG_STATUS_CONTROL_MASK_R8ST);
	// Copy payload.
	for (idx=0 ; idx<R4S8CR_SIGFOX_PAYLOAD_ELECTRICAL_SIZE ; idx++) {
		(ul_payload_update -> ul_payload)[idx] = sigfox_payload_data.frame[idx];
	}
	(*(ul_payload_update -> size)) = R4S8CR_SIGFOX_PAYLOAD_ELECTRICAL_SIZE;
errors:
	return status;
}

/* FILL R4S8CR BUFFER WITH A NEW BYTE (CALLED BY LPUART INTERRUPT).
 * @param rx_byte:	Incoming byte.
 * @return:			None.
 */
void R4S8CR_fill_rx_buffer(uint8_t rx_byte) {
	// Store incoming byte.
	r4s8cr_ctx.reply[r4s8cr_ctx.reply_size] = rx_byte;
	// Manage index.
	r4s8cr_ctx.reply_size = (r4s8cr_ctx.reply_size + 1) % R4S8CR_BUFFER_SIZE_BYTES;
}
