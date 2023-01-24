/*
 * lvrm.c
 *
 *  Created on: 22 jan. 2023
 *      Author: Ludo
 */

#include "dinfox.h"
#include "lpuart.h"
#include "lvrm.h"
#include "node_common.h"
#include "rs485.h"
#include "rs485_common.h"
#include "string.h"

/*** LVRM local structures ***/

typedef enum {
	LVRM_REGISTER_VCOM_MV = DINFOX_REGISTER_LAST,
	LVRM_REGISTER_VOUT_MV,
	LVRM_REGISTER_IOUT_UA,
	LVRM_REGISTER_OUT_EN,
	LVRM_REGISTER_LAST,
} LVRM_register_address_t;

/*** LVRM local macros ***/

#define LVRM_NUMBER_OF_MEASUREMENTS		(DINFOX_NUMBER_OF_COMMON_MEASUREMENTS + LVRM_REGISTER_LAST - DINFOX_REGISTER_LAST)
#define LVRM_STRING_BUFFER_SIZE			16

static const char_t* LVRM_BOARD_NAME = "LVRM";
static const char_t* LVRM_MEASUREMENTS_NAME[LVRM_NUMBER_OF_MEASUREMENTS] = {DINFOX_COMMON_MEASUREMENTS_NAME, "VCOM =", "VOUT =", "IOUT =", "RELAY ="};
//static const char_t* LVRM_MEASUREMENTS_UNITS[LVRM_NUMBER_OF_MEASUREMENTS] = {STRING_NULL, STRING_NULL, STRING_NULL, "°C", "mV", "mV", "mV", "uA", STRING_NULL};

/*** LVRM local structures ***/

typedef struct {
	RS485_address_t rs485_address;
	uint8_t read_index;
	char_t measurements_value[LVRM_NUMBER_OF_MEASUREMENTS][LVRM_STRING_BUFFER_SIZE];
	uint8_t measurements_value_size[LVRM_NUMBER_OF_MEASUREMENTS];
} LVRM_context_t;

/*** LVRM local global variables ***/

static LVRM_context_t lvrm_ctx;

/*** LVRM local functions ***/

/* FLUSH MEASURERMENTS VALUE BUFFER.
 * @param:	None.
 * @return:	None.
 */
void _LVRM_flush_measurements_value(void) {
	// Local variables.
	uint8_t measurement_idx = 0;
	uint8_t idx = 0;
	// Values loop.
	for (measurement_idx=0 ; measurement_idx<LVRM_NUMBER_OF_MEASUREMENTS ; measurement_idx++) {
		lvrm_ctx.measurements_value_size[measurement_idx] = 0;
		for (idx=0 ; idx<LVRM_STRING_BUFFER_SIZE ; idx++) lvrm_ctx.measurements_value[measurement_idx][idx] = STRING_CHAR_NULL;
	}
}

/*** LVRM functions ***/

/* GET NODE BOARD NAME.
 * @param board_name:	Pointer to string that will contain board name.
 * @return status:		Function execution status.
 */
NODE_status_t LVRM_get_board_name(char_t** board_name_ptr) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	// Update pointer.
	(*board_name_ptr) = (char_t*) LVRM_BOARD_NAME;
	return status;
}

/* SET LVRM NODE ADDRESS.
 * @param rs485_address:	RS485 address of the LVRM node.
 * @return:					None.
 */
NODE_status_t LVRM_set_rs485_address(RS485_address_t rs485_address) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	// Check address.
	if (rs485_address > RS485_ADDRESS_LAST) {
		status = NODE_ERROR_RS485_ADDRESS;
		goto errors;
	}
	lvrm_ctx.rs485_address = rs485_address;
errors:
	return status;
}

/* RETRIEVE DATA FROM LVRM NODE.
 * @param:			None.
 * @return status:	Function execution status.
 */
NODE_status_t LVRM_perform_measurements(void) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	LPUART_status_t lpuart1_status = LPUART_SUCCESS;
	RS485_status_t rs485_status = RS485_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	RS485_reply_input_t reply_in;
	RS485_reply_output_t reply_out;
	// Reset buffers.
	_LVRM_flush_measurements_value();
	// Turn RS485 on.
	lpuart1_status = LPUART1_power_on();
	LPUART1_status_check(NODE_ERROR_BASE_LPUART);
	// Common reply parameters.
	reply_in.type = RS485_REPLY_TYPE_RAW;
	reply_in.timeout_ms = 100;
	reply_in.format = STRING_FORMAT_HEXADECIMAL;
	// Hardware version major.
	rs485_status = RS485_send_command(lvrm_ctx.rs485_address, "RS$R=02"); // TODO replace by RS485_read_node_register().
	RS485_status_check(NODE_ERROR_BASE_RS485);
	rs485_status = RS485_wait_reply(&reply_in, &reply_out);
	RS485_status_check(NODE_ERROR_BASE_RS485);
	// Append string.
	string_status = STRING_append_string(lvrm_ctx.measurements_value[0], LVRM_STRING_BUFFER_SIZE, reply_out.raw, &(lvrm_ctx.measurements_value_size[0]));
	STRING_status_check(NODE_ERROR_BASE_STRING);
	string_status = STRING_append_string(lvrm_ctx.measurements_value[0], LVRM_STRING_BUFFER_SIZE, ".", &(lvrm_ctx.measurements_value_size[0]));
	STRING_status_check(NODE_ERROR_BASE_STRING);
	// Hardware version minor.
	rs485_status = RS485_send_command(lvrm_ctx.rs485_address, "RS$R=03");
	RS485_status_check(NODE_ERROR_BASE_RS485);
	rs485_status = RS485_wait_reply(&reply_in, &reply_out);
	RS485_status_check(NODE_ERROR_BASE_RS485);
	// Append string.
	string_status = STRING_append_string(lvrm_ctx.measurements_value[0], LVRM_STRING_BUFFER_SIZE, reply_out.raw, &(lvrm_ctx.measurements_value_size[0]));
	STRING_status_check(NODE_ERROR_BASE_STRING);
errors:
	lvrm_ctx.read_index = 0;
	LPUART1_power_off();
	return status;
}

/* UNSTACK NODE DATA FORMATTED AS STRING.
 * @param measurement_name_ptr:		Pointer to string that will contain next measurement name.
 * @param measurement_value_ptr:	Pointer to string that will contain next measurement value.
 * @return status:					Function execution status.
 */
NODE_status_t LVRM_unstack_string_data(char_t** measurement_name_ptr, char_t** measurement_value_ptr) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	// Check index.
	if (lvrm_ctx.read_index < LVRM_NUMBER_OF_MEASUREMENTS) {
		// Update input pointers.
		(*measurement_name_ptr) = (char_t*) LVRM_MEASUREMENTS_NAME[lvrm_ctx.read_index];
		(*measurement_value_ptr) = (char_t*) lvrm_ctx.measurements_value[lvrm_ctx.read_index];
		// Increment index.
		lvrm_ctx.read_index++;
	}
	else {
		// Return NULL pointers.
		(*measurement_name_ptr) = NULL;
		(*measurement_value_ptr) = NULL;
		// Reset index.
		lvrm_ctx.read_index = 0;
	}
	return status;
}

/* GET NODE SIGFOX PAYLOAD.
 * @param ul_payload:		Pointer that will contain UL payload of the node.
 * @param ul_payload_size:	Pointer to byte that will contain UL payload size.
 * @return status:			Function execution status.
 */
NODE_status_t LVRM_get_sigfox_payload(uint8_t* ul_payload, uint8_t* ul_payload_size) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	return status;
}

/* WRITE LVRM NODE DATA.
 * @param register_address:	Register to write.
 * @param value:			Value to write in register.
 * @return status:			Function execution status.
 */
NODE_status_t LVRM_write(uint8_t register_address, uint8_t value) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	return status;
}

