/*
 * mpmcm.c
 *
 *  Created on: 26 feb. 2023
 *      Author: Ludo
 */

#include "mpmcm.h"

#include "at_bus.h"
#include "common.h"
#include "dinfox.h"
#include "error.h"
#include "mpmcm_reg.h"
#include "node.h"
#include "node_common.h"
#include "string.h"
#include "uhfm.h"
#include "xm.h"

/*** MPMCM local macros ***/

#define MPMCM_NUMBER_OF_ACI_CHANNELS					4

#define MPMCM_SIGFOX_PAYLOAD_MAINS_POWER_FACTOR_SIZE	4
#define MPMCM_SIGFOX_PAYLOAD_MAINS_FREQUENCY_SIZE		6
#define MPMCM_SIGFOX_PAYLOAD_MAINS_VOLTAGE_SIZE			7
#define MPMCM_SIGFOX_PAYLOAD_MAINS_POWER_SIZE			9

#define MPMCM_SIGFOX_PAYLOAD_LOOP_MAX					10

/*** MPMCM local structures ***/

/*******************************************************************/
typedef enum {
	MPMCM_SIGFOX_PAYLOAD_TYPE_STARTUP = 0,
	MPMCM_SIGFOX_PAYLOAD_TYPE_ERROR_STACK,
	MPMCM_SIGFOX_PAYLOAD_TYPE_LAST
} MPMCM_sigfox_payload_type_t;

/*******************************************************************/
typedef union {
	uint8_t frame[MPMCM_SIGFOX_PAYLOAD_MAINS_VOLTAGE_SIZE];
	struct {
		unsigned unused : 3;
		unsigned mvd : 1;
		unsigned ch4d : 1;
		unsigned ch3d : 1;
		unsigned ch2d : 1;
		unsigned ch1d : 1;
		unsigned vrms_min : 16;
		unsigned vrms_mean : 16;
		unsigned vrms_max : 16;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} MPMCM_sigfox_payload_mains_voltage_t;

/*******************************************************************/
typedef union {
	uint8_t frame[MPMCM_SIGFOX_PAYLOAD_MAINS_FREQUENCY_SIZE];
	struct {
		unsigned f_min : 16;
		unsigned f_mean : 16;
		unsigned f_max : 16;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} MPMCM_sigfox_payload_mains_frequency_t;

/*******************************************************************/
typedef union {
	uint8_t frame[MPMCM_SIGFOX_PAYLOAD_MAINS_POWER_SIZE];
	struct {
		unsigned unused : 6;
		unsigned channel_index : 2;
		unsigned pact_mean : 16;
		unsigned pact_max : 16;
		unsigned papp_mean : 16;
		unsigned papp_max : 16;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} MPMCM_sigfox_payload_mains_power_t;

/*******************************************************************/
typedef union {
	uint8_t frame[MPMCM_SIGFOX_PAYLOAD_MAINS_POWER_FACTOR_SIZE];
	struct {
		unsigned unused : 6;
		unsigned channel_index : 2;
		unsigned pf_min : 8;
		unsigned pf_mean : 8;
		unsigned pf_max : 8;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} MPMCM_sigfox_payload_mains_power_factor_t;

/*** MPMCM local global variables ***/

static uint32_t MPMCM_REGISTERS[MPMCM_REG_ADDR_LAST];
static uint8_t mpmcm_por_flag = 1;
static uint8_t mpmcm_mvd_flag = 0;

static const NODE_line_data_t MPMCM_LINE_DATA[MPMCM_LINE_DATA_INDEX_LAST - COMMON_LINE_DATA_INDEX_LAST] = {
	{"VRMS =",  " V",  STRING_FORMAT_DECIMAL, 0, MPMCM_REG_ADDR_CH1_RMS_VOLTAGE_0,    MPMCM_REG_X_0_RUN_MASK, MPMCM_REG_ADDR_CH1_RMS_VOLTAGE_0,    DINFOX_REG_MASK_NONE},
	{"FREQ = ", " Hz", STRING_FORMAT_DECIMAL, 0, MPMCM_REG_ADDR_MAINS_FREQUENCY_0,    MPMCM_REG_X_0_RUN_MASK, MPMCM_REG_ADDR_MAINS_FREQUENCY_0,    DINFOX_REG_MASK_NONE},
	{"PACT1 =", " W",  STRING_FORMAT_DECIMAL, 0, MPMCM_REG_ADDR_CH1_ACTIVE_POWER_0,   MPMCM_REG_X_0_RUN_MASK, MPMCM_REG_ADDR_CH1_ACTIVE_POWER_0,   DINFOX_REG_MASK_NONE},
	{"PAPP1 =", " VA", STRING_FORMAT_DECIMAL, 0, MPMCM_REG_ADDR_CH1_APPARENT_POWER_0, MPMCM_REG_X_0_RUN_MASK, MPMCM_REG_ADDR_CH1_APPARENT_POWER_0, DINFOX_REG_MASK_NONE},
	{"PACT2 =", " W",  STRING_FORMAT_DECIMAL, 0, MPMCM_REG_ADDR_CH2_ACTIVE_POWER_0,   MPMCM_REG_X_0_RUN_MASK, MPMCM_REG_ADDR_CH2_ACTIVE_POWER_0,   DINFOX_REG_MASK_NONE},
	{"PAPP2 =", " VA", STRING_FORMAT_DECIMAL, 0, MPMCM_REG_ADDR_CH2_APPARENT_POWER_0, MPMCM_REG_X_0_RUN_MASK, MPMCM_REG_ADDR_CH2_APPARENT_POWER_0, DINFOX_REG_MASK_NONE},
	{"PACT3 =", " W",  STRING_FORMAT_DECIMAL, 0, MPMCM_REG_ADDR_CH3_ACTIVE_POWER_0,   MPMCM_REG_X_0_RUN_MASK, MPMCM_REG_ADDR_CH3_ACTIVE_POWER_0,   DINFOX_REG_MASK_NONE},
	{"PAPP3 =", " VA", STRING_FORMAT_DECIMAL, 0, MPMCM_REG_ADDR_CH3_APPARENT_POWER_0, MPMCM_REG_X_0_RUN_MASK, MPMCM_REG_ADDR_CH3_APPARENT_POWER_0, DINFOX_REG_MASK_NONE},
	{"PACT4 =", " W",  STRING_FORMAT_DECIMAL, 0, MPMCM_REG_ADDR_CH4_ACTIVE_POWER_0,   MPMCM_REG_X_0_RUN_MASK, MPMCM_REG_ADDR_CH4_ACTIVE_POWER_0,   DINFOX_REG_MASK_NONE},
	{"PAPP4 =", " VA", STRING_FORMAT_DECIMAL, 0, MPMCM_REG_ADDR_CH4_APPARENT_POWER_0, MPMCM_REG_X_0_RUN_MASK, MPMCM_REG_ADDR_CH4_APPARENT_POWER_0, DINFOX_REG_MASK_NONE},
};

#define MPMCM_REG_ERROR_VALUE_CHx \
	/* Active power */ \
	((DINFOX_ELECTRICAL_POWER_ERROR_VALUE << 16) | (DINFOX_ELECTRICAL_POWER_ERROR_VALUE << 0)), \
	((DINFOX_ELECTRICAL_POWER_ERROR_VALUE << 16) | (DINFOX_ELECTRICAL_POWER_ERROR_VALUE << 0)), \
	/* RMS voltage */ \
	((DINFOX_VOLTAGE_ERROR_VALUE << 16) | (DINFOX_VOLTAGE_ERROR_VALUE << 0)), \
	((DINFOX_VOLTAGE_ERROR_VALUE << 16) | (DINFOX_VOLTAGE_ERROR_VALUE << 0)), \
	/* RMS current */ \
	((DINFOX_CURRENT_ERROR_VALUE << 16) | (DINFOX_CURRENT_ERROR_VALUE << 0)), \
	((DINFOX_CURRENT_ERROR_VALUE << 16) | (DINFOX_CURRENT_ERROR_VALUE << 0)), \
	/* Apparent power */ \
	((DINFOX_ELECTRICAL_POWER_ERROR_VALUE << 16) | (DINFOX_ELECTRICAL_POWER_ERROR_VALUE << 0)), \
	((DINFOX_ELECTRICAL_POWER_ERROR_VALUE << 16) | (DINFOX_ELECTRICAL_POWER_ERROR_VALUE << 0)), \
	/* Power factor */ \
	((DINFOX_POWER_FACTOR_ERROR_VALUE << 16) | (DINFOX_POWER_FACTOR_ERROR_VALUE << 0)), \
	((DINFOX_POWER_FACTOR_ERROR_VALUE << 16) | (DINFOX_POWER_FACTOR_ERROR_VALUE << 0)), \

static const uint32_t MPMCM_REG_ERROR_VALUE[MPMCM_REG_ADDR_LAST] = {
	COMMON_REG_ERROR_VALUE
	0x00000000,
	0x00000000,
	((DINFOX_MAINS_FREQUENCY_ERROR_VALUE << 16) | (DINFOX_MAINS_FREQUENCY_ERROR_VALUE << 0)),
	((DINFOX_MAINS_FREQUENCY_ERROR_VALUE << 16) | (DINFOX_MAINS_FREQUENCY_ERROR_VALUE << 0)),
	MPMCM_REG_ERROR_VALUE_CHx
	MPMCM_REG_ERROR_VALUE_CHx
	MPMCM_REG_ERROR_VALUE_CHx
	MPMCM_REG_ERROR_VALUE_CHx
};

static const uint8_t MPMCM_REG_LIST_SIGFOX_PAYLOAD_MAINS_VOLTAGE[] = {
	MPMCM_REG_ADDR_STATUS,
	MPMCM_REG_ADDR_CH1_RMS_VOLTAGE_0,
	MPMCM_REG_ADDR_CH1_RMS_VOLTAGE_1
};

static const uint8_t MPMCM_REG_LIST_SIGFOX_PAYLOAD_MAINS_FREQUENCY[] = {
	MPMCM_REG_ADDR_MAINS_FREQUENCY_0,
	MPMCM_REG_ADDR_MAINS_FREQUENCY_1,
};

static const uint8_t MPMCM_REG_LIST_SIGFOX_PAYLOAD_MAINS_POWER[] = {
	MPMCM_REG_ADDR_CH1_ACTIVE_POWER_0,
	MPMCM_REG_ADDR_CH1_ACTIVE_POWER_1,
	MPMCM_REG_ADDR_CH1_APPARENT_POWER_0,
	MPMCM_REG_ADDR_CH1_APPARENT_POWER_1
};

static const uint8_t MPMCM_REG_LIST_SIGFOX_PAYLOAD_MAINS_POWER_FACTOR[] = {
	MPMCM_REG_ADDR_CH1_POWER_FACTOR_0,
	MPMCM_REG_ADDR_CH1_POWER_FACTOR_1,
};

static const MPMCM_sigfox_payload_type_t MPMCM_SIGFOX_PAYLOAD_PATTERN[] = {
	MPMCM_SIGFOX_PAYLOAD_TYPE_STARTUP,
	MPMCM_SIGFOX_PAYLOAD_TYPE_ERROR_STACK
};

/*** MPMCM functions ***/

/*******************************************************************/
NODE_status_t MPMCM_write_line_data(NODE_line_data_write_t* line_data_write, NODE_access_status_t* write_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	// Check common range.
	if ((line_data_write -> line_data_index) < COMMON_LINE_DATA_INDEX_LAST) {
		// Call common function.
		status = COMMON_write_line_data(line_data_write, write_status);
	}
	else {
		// Remove offset.
		(line_data_write -> line_data_index) -= COMMON_LINE_DATA_INDEX_LAST;
		// Call common function.
		status = XM_write_line_data(line_data_write, (NODE_line_data_t*) MPMCM_LINE_DATA, (uint32_t*) MPMCM_REG_WRITE_TIMEOUT_MS, write_status);
	}
	return status;
}

/*******************************************************************/
NODE_status_t MPMCM_read_line_data(NODE_line_data_read_t* line_data_read, NODE_access_status_t* read_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	XM_node_registers_t node_reg;
	STRING_status_t string_status = STRING_SUCCESS;
	uint32_t field_value = 0;
	char_t field_str[STRING_DIGIT_FUNCTION_SIZE];
	uint8_t str_data_idx = 0;
	uint8_t reg_addr = 0;
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
	if ((line_data_read -> line_data_index) >= MPMCM_LINE_DATA_INDEX_LAST) {
		status = NODE_ERROR_LINE_DATA_INDEX;
		goto errors;
	}
	// Build node registers structure.
	node_reg.value = (uint32_t*) MPMCM_REGISTERS;
	node_reg.error = (uint32_t*) MPMCM_REG_ERROR_VALUE;
	// Check common range.
	if ((line_data_read -> line_data_index) < COMMON_LINE_DATA_INDEX_LAST) {
		// Call common function.
		status = COMMON_read_line_data(line_data_read, &node_reg, read_status);
		if (status != NODE_SUCCESS) goto errors;
	}
	else {
		// Compute specific string data index and register address.
		str_data_idx = ((line_data_read -> line_data_index) - COMMON_LINE_DATA_INDEX_LAST);
		reg_addr = MPMCM_LINE_DATA[str_data_idx].read_reg_addr;
		// Add data name.
		NODE_append_name_string((char_t*) MPMCM_LINE_DATA[str_data_idx].name);
		buffer_size = 0;
		// Reset result to error.
		NODE_flush_string_value();
		NODE_append_value_string((char_t*) NODE_ERROR_STRING);
		// Update register.
		status = XM_read_register((line_data_read -> node_addr), reg_addr, MPMCM_REG_ERROR_VALUE[reg_addr], &(MPMCM_REGISTERS[reg_addr]), read_status);
		if ((status != NODE_SUCCESS) || ((read_status -> all) != 0)) goto errors;
		// Compute field.
		field_value = DINFOX_read_field(MPMCM_REGISTERS[reg_addr], MPMCM_LINE_DATA[str_data_idx].read_field_mask);
		// Check index.
		switch (line_data_read -> line_data_index) {
		case MPMCM_LINE_DATA_INDEX_VRMS:
			// Check error value.
			if (field_value != DINFOX_VOLTAGE_ERROR_VALUE) {
				// Convert to 5 digits string.
				string_status = STRING_value_to_5_digits_string(DINFOX_get_mv(field_value), (char_t*) field_str);
				STRING_exit_error(NODE_ERROR_BASE_STRING);
				// Add string.
				NODE_flush_string_value();
				NODE_append_value_string(field_str);
				// Add unit.
				NODE_append_value_string((char_t*) MPMCM_LINE_DATA[str_data_idx].unit);
			}
			break;
		case MPMCM_LINE_DATA_INDEX_FREQ:
			// Check error value.
			if (field_value != DINFOX_MAINS_FREQUENCY_ERROR_VALUE) {
				// Convert to mHz then to 5 digits string.
				string_status = STRING_value_to_5_digits_string((field_value * 10), (char_t*) field_str);
				STRING_exit_error(NODE_ERROR_BASE_STRING);
				// Add string.
				NODE_flush_string_value();
				NODE_append_value_string(field_str);
				// Add unit.
				NODE_append_value_string((char_t*) MPMCM_LINE_DATA[str_data_idx].unit);
			}
			break;
		case MPMCM_LINE_DATA_INDEX_CH1_PACT:
		case MPMCM_LINE_DATA_INDEX_CH1_PAPP:
		case MPMCM_LINE_DATA_INDEX_CH2_PACT:
		case MPMCM_LINE_DATA_INDEX_CH2_PAPP:
		case MPMCM_LINE_DATA_INDEX_CH3_PACT:
		case MPMCM_LINE_DATA_INDEX_CH3_PAPP:
		case MPMCM_LINE_DATA_INDEX_CH4_PACT:
		case MPMCM_LINE_DATA_INDEX_CH4_PAPP:
			// Check error value.
			if (field_value != DINFOX_ELECTRICAL_POWER_ERROR_VALUE) {
				// Convert to 5 digits string.
				string_status = STRING_value_to_5_digits_string(DINFOX_get_mw(field_value), (char_t*) field_str);
				STRING_exit_error(NODE_ERROR_BASE_STRING);
				// Add string.
				NODE_flush_string_value();
				NODE_append_value_string(field_str);
				// Add unit.
				NODE_append_value_string((char_t*) MPMCM_LINE_DATA[str_data_idx].unit);
			}
			break;
		default:
			NODE_flush_string_value();
			NODE_append_value_int32(field_value, STRING_FORMAT_HEXADECIMAL, 1);
			break;
		}
	}
errors:
	return status;
}

/*******************************************************************/
NODE_status_t MPMCM_build_sigfox_ul_payload(NODE_ul_payload_t* node_ul_payload) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	XM_node_registers_t node_reg;
	uint32_t loop_count = 0;
	// Check parameters.
	if (node_ul_payload == NULL) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	if (((node_ul_payload -> node) == NULL) || ((node_ul_payload -> ul_payload) == NULL) || ((node_ul_payload -> size) == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Build node registers structure.
	node_reg.value = (uint32_t*) MPMCM_REGISTERS;
	node_reg.error = (uint32_t*) MPMCM_REG_ERROR_VALUE;
	// Reset payload size.
	(*(node_ul_payload -> size)) = 0;
	// Main loop.
	do {
		// Check payload type.
		switch (MPMCM_SIGFOX_PAYLOAD_PATTERN[node_ul_payload -> node -> radio_transmission_count]) {
		case MPMCM_SIGFOX_PAYLOAD_TYPE_STARTUP:
			// Check flag.
			if ((node_ul_payload -> node -> startup_data_sent) == 0) {
				// Use common format.
				status = COMMON_build_sigfox_payload_startup(node_ul_payload, &node_reg);
				if (status != NODE_SUCCESS) goto errors;
				// Update flag.
				(node_ul_payload -> node -> startup_data_sent) = 1;
			}
			break;
		case MPMCM_SIGFOX_PAYLOAD_TYPE_ERROR_STACK:
			// Use common format.
			status = COMMON_build_sigfox_payload_error_stack(node_ul_payload, &node_reg);
			if (status != NODE_SUCCESS) goto errors;
			break;
		default:
			status = NODE_ERROR_SIGFOX_PAYLOAD_TYPE;
			goto errors;
		}
		// Increment transmission count.
		(node_ul_payload -> node -> radio_transmission_count) = ((node_ul_payload -> node -> radio_transmission_count) + 1) % (sizeof(MPMCM_SIGFOX_PAYLOAD_PATTERN));
		// Exit in case of loop error.
		loop_count++;
		if (loop_count > MPMCM_SIGFOX_PAYLOAD_LOOP_MAX) break;
	}
	while ((*(node_ul_payload -> size)) == 0);
errors:
	return status;
}

/*******************************************************************/
NODE_status_t MPMCM_radio_process(NODE_address_t mpmcm_node_addr, NODE_address_t uhfm_node_addr) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	NODE_access_status_t node_access_status;
	NODE_access_status_t chxs_access_status;
	XM_node_registers_t node_reg;
	XM_registers_list_t reg_list;
	MPMCM_sigfox_payload_mains_voltage_t sigfox_payload_mains_voltage;
	MPMCM_sigfox_payload_mains_frequency_t sigfox_payload_mains_frequency;
	MPMCM_sigfox_payload_mains_power_t sigfox_payload_mains_power;
	MPMCM_sigfox_payload_mains_power_factor_t sigfox_payload_mains_power_factor;
	NODE_dinfox_ul_payload_t dinfox_ul_payload;
	UHFM_sigfox_message_t sigfox_message;
	uint8_t addr_list[4];
	uint32_t control_1 = 0;
	uint8_t channel_idx;
	uint32_t reg_offset = 0;
	uint8_t idx = 0;
	// Build node registers structure.
	node_reg.value = (uint32_t*) MPMCM_REGISTERS;
	node_reg.error = (uint32_t*) MPMCM_REG_ERROR_VALUE;
	// Add board ID and node address.
	dinfox_ul_payload.board_id = DINFOX_BOARD_ID_MPMCM;
	dinfox_ul_payload.node_addr = mpmcm_node_addr;
	// Common sigfox messages parameters.
	sigfox_message.ul_payload = (uint8_t*) dinfox_ul_payload.frame;
	sigfox_message.bidirectional_flag = 0;
	// Reset MVD flag.
	sigfox_payload_mains_voltage.mvd = 0;
	// Store accumulated data of all channels (synchronization reset in case of POR).
	control_1 |= MPMCM_REG_CONTROL_1_MASK_CH1S;
	control_1 |= MPMCM_REG_CONTROL_1_MASK_CH2S;
	control_1 |= MPMCM_REG_CONTROL_1_MASK_CH3S;
	control_1 |= MPMCM_REG_CONTROL_1_MASK_CH4S;
	control_1 |= MPMCM_REG_CONTROL_1_MASK_FRQS;
	status = XM_write_register(mpmcm_node_addr, MPMCM_REG_ADDR_CONTROL_1, control_1, control_1, AT_BUS_DEFAULT_TIMEOUT_MS, &chxs_access_status);
	if (status != NODE_SUCCESS) goto errors;
	// Do not send frames on POR.
	if (mpmcm_por_flag != 0) goto errors;
	// Build registers list for mains voltage.
	reg_list.addr_list = (uint8_t*) MPMCM_REG_LIST_SIGFOX_PAYLOAD_MAINS_VOLTAGE;
	reg_list.size = sizeof(MPMCM_REG_LIST_SIGFOX_PAYLOAD_MAINS_VOLTAGE);
	// Reset data registers.
	XM_reset_registers(&reg_list, &node_reg);
	// Check write status.
	if (chxs_access_status.all == 0) {
		// Read related registers.
		status = XM_read_registers(mpmcm_node_addr, &reg_list, &node_reg);
		if (status != NODE_SUCCESS) goto errors;
	}
	// Build mains voltage frame.
	sigfox_payload_mains_voltage.unused = 0;
	sigfox_payload_mains_voltage.mvd =  DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_STATUS], MPMCM_REG_STATUS_MASK_MVD);
	sigfox_payload_mains_voltage.ch4d = DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_STATUS], MPMCM_REG_STATUS_MASK_CH4D);
	sigfox_payload_mains_voltage.ch3d = DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_STATUS], MPMCM_REG_STATUS_MASK_CH3D);
	sigfox_payload_mains_voltage.ch2d = DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_STATUS], MPMCM_REG_STATUS_MASK_CH2D);
	sigfox_payload_mains_voltage.ch1d = DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_STATUS], MPMCM_REG_STATUS_MASK_CH1D);
	sigfox_payload_mains_voltage.vrms_min =  DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_CH1_RMS_VOLTAGE_1 + reg_offset], MPMCM_REG_X_1_MIN_MASK);
	sigfox_payload_mains_voltage.vrms_mean = DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_CH1_RMS_VOLTAGE_0 + reg_offset], MPMCM_REG_X_0_MEAN_MASK);
	sigfox_payload_mains_voltage.vrms_max =  DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_CH1_RMS_VOLTAGE_1 + reg_offset], MPMCM_REG_X_1_MAX_MASK);
	// Copy payload and update size.
	for (idx=0 ; idx<MPMCM_SIGFOX_PAYLOAD_MAINS_VOLTAGE_SIZE ; idx++) {
		dinfox_ul_payload.node_data[idx] = sigfox_payload_mains_voltage.frame[idx];
	}
	sigfox_message.ul_payload_size = (NODE_DINFOX_PAYLOAD_HEADER_SIZE + MPMCM_SIGFOX_PAYLOAD_MAINS_VOLTAGE_SIZE);
	// Send message.
	status = UHFM_send_sigfox_message(uhfm_node_addr, &sigfox_message, &node_access_status);
	if (status != NODE_SUCCESS) goto errors;
	NODE_check_access_status(uhfm_node_addr);
	// Do not send any other frame if there was an error during first access.
	if (chxs_access_status.all != 0) goto errors;
	// Do not send any other frame if mains voltage was not present 2 consecutive times.
	if ((sigfox_payload_mains_voltage.mvd == 0) && (mpmcm_mvd_flag == 0)) goto errors;
	// Build registers list for mains frequency.
	reg_list.addr_list = (uint8_t*) MPMCM_REG_LIST_SIGFOX_PAYLOAD_MAINS_FREQUENCY;
	reg_list.size = sizeof(MPMCM_REG_LIST_SIGFOX_PAYLOAD_MAINS_FREQUENCY);
	// Reset data registers.
	XM_reset_registers(&reg_list, &node_reg);
	// Read related registers.
	status = XM_read_registers(mpmcm_node_addr, &reg_list, &node_reg);
	if (status != NODE_SUCCESS) goto errors;
	// Build mains frequency frame.
	sigfox_payload_mains_frequency.f_min =  DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_MAINS_FREQUENCY_1 + reg_offset], MPMCM_REG_X_1_MIN_MASK);
	sigfox_payload_mains_frequency.f_mean = DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_MAINS_FREQUENCY_0 + reg_offset], MPMCM_REG_X_0_MEAN_MASK);
	sigfox_payload_mains_frequency.f_max =  DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_MAINS_FREQUENCY_1 + reg_offset], MPMCM_REG_X_1_MAX_MASK);
	// Copy payload and update size.
	for (idx=0 ; idx<MPMCM_SIGFOX_PAYLOAD_MAINS_VOLTAGE_SIZE ; idx++) {
		dinfox_ul_payload.node_data[idx] = sigfox_payload_mains_frequency.frame[idx];
	}
	sigfox_message.ul_payload_size = (NODE_DINFOX_PAYLOAD_HEADER_SIZE + MPMCM_SIGFOX_PAYLOAD_MAINS_FREQUENCY_SIZE);
	// Send message.
	status = UHFM_send_sigfox_message(uhfm_node_addr, &sigfox_message, &node_access_status);
	if (status != NODE_SUCCESS) goto errors;
	NODE_check_access_status(uhfm_node_addr);

	// Use dynamic list for channels.
	reg_list.addr_list = (uint8_t*) addr_list;
	// Channels loop.
	for (channel_idx=0 ; channel_idx<MPMCM_NUMBER_OF_ACI_CHANNELS ; channel_idx++) {
		// Check detect flag.
		if ((MPMCM_REGISTERS[MPMCM_REG_ADDR_STATUS] & (0b1 << ((channel_idx << 1) + 1))) == 0) continue;
		// Compute data registers offset according to selected channel.
		reg_offset = (channel_idx * MPMCM_NUMBER_OF_REG_PER_DATA);
		// Build registers list for mains power frame.
		for (idx=0 ; idx<sizeof(MPMCM_REG_LIST_SIGFOX_PAYLOAD_MAINS_POWER) ; idx++) {
			addr_list[idx] = MPMCM_REG_LIST_SIGFOX_PAYLOAD_MAINS_POWER[idx] + reg_offset;
		}
		reg_list.size = sizeof(MPMCM_REG_LIST_SIGFOX_PAYLOAD_MAINS_POWER);
		// Reset data registers.
		XM_reset_registers(&reg_list, &node_reg);
		// Read related registers.
		status = XM_read_registers(mpmcm_node_addr, &reg_list, &node_reg);
		if (status != NODE_SUCCESS) goto errors;
		// Build mains power frame.
		sigfox_payload_mains_power.unused = 0;
		sigfox_payload_mains_power.channel_index = channel_idx;
		sigfox_payload_mains_power.pact_mean = DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_CH1_ACTIVE_POWER_0 + reg_offset], MPMCM_REG_X_0_MEAN_MASK);
		sigfox_payload_mains_power.pact_max =  DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_CH1_ACTIVE_POWER_1 + reg_offset], MPMCM_REG_X_1_MAX_MASK);
		sigfox_payload_mains_power.papp_mean = DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_CH1_APPARENT_POWER_0 + reg_offset], MPMCM_REG_X_0_MEAN_MASK);
		sigfox_payload_mains_power.papp_max =  DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_CH1_APPARENT_POWER_1 + reg_offset], MPMCM_REG_X_1_MAX_MASK);
		// Copy payload and update size.
		for (idx=0 ; idx<MPMCM_SIGFOX_PAYLOAD_MAINS_POWER_SIZE ; idx++) {
			dinfox_ul_payload.node_data[idx] = sigfox_payload_mains_power.frame[idx];
		}
		sigfox_message.ul_payload_size = (NODE_DINFOX_PAYLOAD_HEADER_SIZE + MPMCM_SIGFOX_PAYLOAD_MAINS_POWER_SIZE);
		// Send message.
		status = UHFM_send_sigfox_message(uhfm_node_addr, &sigfox_message, &node_access_status);
		if (status != NODE_SUCCESS) goto errors;
		NODE_check_access_status(uhfm_node_addr);
		// Build registers list for power factor frame.
		for (idx=0 ; idx<sizeof(MPMCM_REG_LIST_SIGFOX_PAYLOAD_MAINS_POWER_FACTOR) ; idx++) {
			addr_list[idx] = MPMCM_REG_LIST_SIGFOX_PAYLOAD_MAINS_POWER_FACTOR[idx] + reg_offset;
		}
		reg_list.size = sizeof(MPMCM_REG_LIST_SIGFOX_PAYLOAD_MAINS_POWER_FACTOR);
		// Reset data registers.
		XM_reset_registers(&reg_list, &node_reg);
		// Read related registers.
		status = XM_read_registers(mpmcm_node_addr, &reg_list, &node_reg);
		if (status != NODE_SUCCESS) goto errors;
		// Build mains power factor frame.
		sigfox_payload_mains_power_factor.unused = 0;
		sigfox_payload_mains_power_factor.channel_index = channel_idx;
		sigfox_payload_mains_power_factor.pf_min =  DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_CH1_POWER_FACTOR_1 + reg_offset], MPMCM_REG_X_1_MIN_MASK);
		sigfox_payload_mains_power_factor.pf_mean = DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_CH1_POWER_FACTOR_0 + reg_offset], MPMCM_REG_X_0_MEAN_MASK);
		sigfox_payload_mains_power_factor.pf_max =  DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_CH1_POWER_FACTOR_1 + reg_offset], MPMCM_REG_X_1_MAX_MASK);
		// Copy payload and update size.
		for (idx=0 ; idx<MPMCM_SIGFOX_PAYLOAD_MAINS_POWER_FACTOR_SIZE ; idx++) {
			dinfox_ul_payload.node_data[idx] = sigfox_payload_mains_power_factor.frame[idx];
		}
		sigfox_message.ul_payload_size = (NODE_DINFOX_PAYLOAD_HEADER_SIZE + MPMCM_SIGFOX_PAYLOAD_MAINS_POWER_FACTOR_SIZE);
		// Send message.
		status = UHFM_send_sigfox_message(uhfm_node_addr, &sigfox_message, &node_access_status);
		if (status != NODE_SUCCESS) goto errors;
		NODE_check_access_status(uhfm_node_addr);
	}
errors:
	// Clear POR flag and update MVD flag.
	mpmcm_por_flag = 0;
	mpmcm_mvd_flag = sigfox_payload_mains_voltage.mvd;
	return status;
}
