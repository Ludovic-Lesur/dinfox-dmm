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

#define MPMCM_NUMBER_OF_ACI_CHANNELS						4
#define MPMCM_TIC_CHANNEL_INDEX								MPMCM_NUMBER_OF_ACI_CHANNELS

#define MPMCM_SIGFOX_UL_PAYLOAD_STATUS_SIZE					1
#define MPMCM_SIGFOX_UL_PAYLOAD_MAINS_POWER_FACTOR_SIZE		4
#define MPMCM_SIGFOX_UL_PAYLOAD_MAINS_ENERGY_SIZE			5
#define MPMCM_SIGFOX_UL_PAYLOAD_MAINS_FREQUENCY_SIZE		6
#define MPMCM_SIGFOX_UL_PAYLOAD_MAINS_VOLTAGE_SIZE			7
#define MPMCM_SIGFOX_UL_PAYLOAD_MAINS_POWER_SIZE			9

/*** MPMCM local structures ***/

/*******************************************************************/
typedef union {
	uint8_t frame[MPMCM_SIGFOX_UL_PAYLOAD_STATUS_SIZE];
	struct {
		unsigned unused : 2;
		unsigned mvd : 1;
		unsigned ticd : 1;
		unsigned ch4d : 1;
		unsigned ch3d : 1;
		unsigned ch2d : 1;
		unsigned ch1d : 1;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} MPMCM_sigfox_ul_payload_status_t;

/*******************************************************************/
typedef union {
	uint8_t frame[MPMCM_SIGFOX_UL_PAYLOAD_MAINS_VOLTAGE_SIZE];
	struct {
		unsigned unused : 5;
		unsigned channel_index : 3;
		unsigned vrms_min : 16;
		unsigned vrms_mean : 16;
		unsigned vrms_max : 16;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} MPMCM_sigfox_ul_payload_mains_voltage_t;

/*******************************************************************/
typedef union {
	uint8_t frame[MPMCM_SIGFOX_UL_PAYLOAD_MAINS_FREQUENCY_SIZE];
	struct {
		unsigned f_min : 16;
		unsigned f_mean : 16;
		unsigned f_max : 16;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} MPMCM_sigfox_ul_payload_mains_frequency_t;

/*******************************************************************/
typedef union {
	uint8_t frame[MPMCM_SIGFOX_UL_PAYLOAD_MAINS_POWER_SIZE];
	struct {
		unsigned unused : 5;
		unsigned channel_index : 3;
		unsigned pact_mean : 16;
		unsigned pact_max : 16;
		unsigned papp_mean : 16;
		unsigned papp_max : 16;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} MPMCM_sigfox_ul_payload_mains_power_t;

/*******************************************************************/
typedef union {
	uint8_t frame[MPMCM_SIGFOX_UL_PAYLOAD_MAINS_POWER_FACTOR_SIZE];
	struct {
		unsigned unused : 5;
		unsigned channel_index : 3;
		unsigned pf_min : 8;
		unsigned pf_mean : 8;
		unsigned pf_max : 8;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} MPMCM_sigfox_ul_payload_mains_power_factor_t;

/*******************************************************************/
typedef union {
	uint8_t frame[MPMCM_SIGFOX_UL_PAYLOAD_MAINS_ENERGY_SIZE];
	struct {
		unsigned unused : 5;
		unsigned channel_index : 3;
		unsigned eact : 16;
		unsigned eapp : 16;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} MPMCM_sigfox_ul_payload_mains_energy_t;

/*** MPMCM global variables ***/

/*******************************************************************/
#define MPMCM_REG_WRITE_TIMEOUT_CHx \
	/* Active power */ \
	AT_BUS_DEFAULT_TIMEOUT_MS, \
	AT_BUS_DEFAULT_TIMEOUT_MS, \
	/* RMS voltage */ \
	AT_BUS_DEFAULT_TIMEOUT_MS, \
	AT_BUS_DEFAULT_TIMEOUT_MS, \
	/* RMS current */ \
	AT_BUS_DEFAULT_TIMEOUT_MS, \
	AT_BUS_DEFAULT_TIMEOUT_MS, \
	/* Apparent power */ \
	AT_BUS_DEFAULT_TIMEOUT_MS, \
	AT_BUS_DEFAULT_TIMEOUT_MS, \
	/* Power factor */ \
	AT_BUS_DEFAULT_TIMEOUT_MS, \
	AT_BUS_DEFAULT_TIMEOUT_MS, \
	/* Energy */ \
	AT_BUS_DEFAULT_TIMEOUT_MS, \

const uint32_t MPMCM_REG_WRITE_TIMEOUT_MS[MPMCM_REG_ADDR_LAST] = {
	COMMON_REG_WRITE_TIMEOUT_MS_LIST
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	5000,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	MPMCM_REG_WRITE_TIMEOUT_CHx
	MPMCM_REG_WRITE_TIMEOUT_CHx
	MPMCM_REG_WRITE_TIMEOUT_CHx
	MPMCM_REG_WRITE_TIMEOUT_CHx
	MPMCM_REG_WRITE_TIMEOUT_CHx
};

/*** MPMCM local global variables ***/

static uint32_t MPMCM_REGISTERS[MPMCM_REG_ADDR_LAST];
static uint8_t mpmcm_por_flag = 1;
static uint8_t mpmcm_mvd_flag = 0;
static uint8_t mpmcm_ticd_flag = 0;

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
	/* Energy */ \
	((DINFOX_ELECTRICAL_ENERGY_ERROR_VALUE << 16) | (DINFOX_ELECTRICAL_ENERGY_ERROR_VALUE << 0)), \

static const uint32_t MPMCM_REG_ERROR_VALUE[MPMCM_REG_ADDR_LAST] = {
	COMMON_REG_ERROR_VALUE_LIST
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	((DINFOX_MAINS_FREQUENCY_ERROR_VALUE << 16) | (DINFOX_MAINS_FREQUENCY_ERROR_VALUE << 0)),
	((DINFOX_MAINS_FREQUENCY_ERROR_VALUE << 16) | (DINFOX_MAINS_FREQUENCY_ERROR_VALUE << 0)),
	MPMCM_REG_ERROR_VALUE_CHx
	MPMCM_REG_ERROR_VALUE_CHx
	MPMCM_REG_ERROR_VALUE_CHx
	MPMCM_REG_ERROR_VALUE_CHx
	MPMCM_REG_ERROR_VALUE_CHx
};

static const XM_node_registers_t MPMCM_NODE_REGISTERS = {
	.value = (uint32_t*) MPMCM_REGISTERS,
	.error = (uint32_t*) MPMCM_REG_ERROR_VALUE,
};

static const NODE_line_data_t MPMCM_LINE_DATA[MPMCM_LINE_DATA_INDEX_LAST - COMMON_LINE_DATA_INDEX_LAST] = {
	{"VRMS =",  " V",  STRING_FORMAT_DECIMAL, 0, MPMCM_REG_ADDR_CH1_RMS_VOLTAGE_0,    MPMCM_REG_MASK_RUN, MPMCM_REG_ADDR_CONTROL_1, DINFOX_REG_MASK_NONE},
	{"FREQ = ", " Hz", STRING_FORMAT_DECIMAL, 0, MPMCM_REG_ADDR_MAINS_FREQUENCY_0,    MPMCM_REG_MASK_RUN, MPMCM_REG_ADDR_CONTROL_1, DINFOX_REG_MASK_NONE},
	{"PACT1 =", " W",  STRING_FORMAT_DECIMAL, 0, MPMCM_REG_ADDR_CH1_ACTIVE_POWER_0,   MPMCM_REG_MASK_RUN, MPMCM_REG_ADDR_CONTROL_1, DINFOX_REG_MASK_NONE},
	{"PAPP1 =", " VA", STRING_FORMAT_DECIMAL, 0, MPMCM_REG_ADDR_CH1_APPARENT_POWER_0, MPMCM_REG_MASK_RUN, MPMCM_REG_ADDR_CONTROL_1, DINFOX_REG_MASK_NONE},
	{"PACT2 =", " W",  STRING_FORMAT_DECIMAL, 0, MPMCM_REG_ADDR_CH2_ACTIVE_POWER_0,   MPMCM_REG_MASK_RUN, MPMCM_REG_ADDR_CONTROL_1, DINFOX_REG_MASK_NONE},
	{"PAPP2 =", " VA", STRING_FORMAT_DECIMAL, 0, MPMCM_REG_ADDR_CH2_APPARENT_POWER_0, MPMCM_REG_MASK_RUN, MPMCM_REG_ADDR_CONTROL_1, DINFOX_REG_MASK_NONE},
	{"PACT3 =", " W",  STRING_FORMAT_DECIMAL, 0, MPMCM_REG_ADDR_CH3_ACTIVE_POWER_0,   MPMCM_REG_MASK_RUN, MPMCM_REG_ADDR_CONTROL_1, DINFOX_REG_MASK_NONE},
	{"PAPP3 =", " VA", STRING_FORMAT_DECIMAL, 0, MPMCM_REG_ADDR_CH3_APPARENT_POWER_0, MPMCM_REG_MASK_RUN, MPMCM_REG_ADDR_CONTROL_1, DINFOX_REG_MASK_NONE},
	{"PACT4 =", " W",  STRING_FORMAT_DECIMAL, 0, MPMCM_REG_ADDR_CH4_ACTIVE_POWER_0,   MPMCM_REG_MASK_RUN, MPMCM_REG_ADDR_CONTROL_1, DINFOX_REG_MASK_NONE},
	{"PAPP4 =", " VA", STRING_FORMAT_DECIMAL, 0, MPMCM_REG_ADDR_CH4_APPARENT_POWER_0, MPMCM_REG_MASK_RUN, MPMCM_REG_ADDR_CONTROL_1, DINFOX_REG_MASK_NONE},
	{"PACT5 =", " W",  STRING_FORMAT_DECIMAL, 0, MPMCM_REG_ADDR_TIC_ACTIVE_POWER_0,   MPMCM_REG_MASK_RUN, MPMCM_REG_ADDR_CONTROL_1, DINFOX_REG_MASK_NONE},
	{"PAPP5 =", " VA", STRING_FORMAT_DECIMAL, 0, MPMCM_REG_ADDR_TIC_APPARENT_POWER_0, MPMCM_REG_MASK_RUN, MPMCM_REG_ADDR_CONTROL_1, DINFOX_REG_MASK_NONE},
};

static const uint8_t MPMCM_REG_LIST_SIGFOX_UL_PAYLOAD_MAINS_VOLTAGE[] = {
	MPMCM_REG_ADDR_STATUS_1,
	MPMCM_REG_ADDR_CH1_RMS_VOLTAGE_0,
	MPMCM_REG_ADDR_CH1_RMS_VOLTAGE_1
};

static const uint8_t MPMCM_REG_LIST_SIGFOX_UL_PAYLOAD_MAINS_FREQUENCY[] = {
	MPMCM_REG_ADDR_MAINS_FREQUENCY_0,
	MPMCM_REG_ADDR_MAINS_FREQUENCY_1,
};

static const uint8_t MPMCM_REG_LIST_SIGFOX_UL_PAYLOAD_MAINS_POWER[] = {
	MPMCM_REG_ADDR_CH1_ACTIVE_POWER_0,
	MPMCM_REG_ADDR_CH1_ACTIVE_POWER_1,
	MPMCM_REG_ADDR_CH1_APPARENT_POWER_0,
	MPMCM_REG_ADDR_CH1_APPARENT_POWER_1
};

static const uint8_t MPMCM_REG_LIST_SIGFOX_UL_PAYLOAD_MAINS_POWER_FACTOR[] = {
	MPMCM_REG_ADDR_CH1_POWER_FACTOR_0,
	MPMCM_REG_ADDR_CH1_POWER_FACTOR_1,
};

static const uint8_t MPMCM_REG_LIST_SIGFOX_UL_PAYLOAD_MAINS_ENERGY[] = {
	MPMCM_REG_ADDR_CH1_ENERGY,
};

/*** MPMCM local functions ***/

/*******************************************************************/
static NODE_status_t _MPMCM_send_status_frame(NODE_address_t mpmcm_node_addr, NODE_address_t uhfm_node_addr) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	NODE_access_status_t access_status;
	NODE_dinfox_ul_payload_t dinfox_ul_payload;
	MPMCM_sigfox_ul_payload_status_t sigfox_ul_payload_status;
	UHFM_sigfox_message_t sigfox_message;
	uint8_t idx = 0;
	// Read status register for status frame.
	status = XM_read_register(mpmcm_node_addr, MPMCM_REG_ADDR_STATUS_1, (XM_node_registers_t*) &MPMCM_NODE_REGISTERS, &access_status);
	if ((status != NODE_SUCCESS) || (access_status.flags != 0)) goto errors;
	// Build status frame.
	sigfox_ul_payload_status.frame[0] = (uint8_t) (MPMCM_REGISTERS[MPMCM_REG_ADDR_STATUS_1] & 0x3F);
	// Add header and copy payload
	dinfox_ul_payload.board_id = DINFOX_BOARD_ID_MPMCM;
	dinfox_ul_payload.node_addr = mpmcm_node_addr;
	for (idx=0 ; idx<MPMCM_SIGFOX_UL_PAYLOAD_STATUS_SIZE ; idx++) dinfox_ul_payload.node_data[idx] = sigfox_ul_payload_status.frame[idx];
	// Set parameters.
	sigfox_message.ul_payload = (uint8_t*) dinfox_ul_payload.frame;
	sigfox_message.ul_payload_size = (NODE_DINFOX_PAYLOAD_HEADER_SIZE + MPMCM_SIGFOX_UL_PAYLOAD_STATUS_SIZE);
	sigfox_message.bidirectional_flag = 0;
	// Send message.
	status = UHFM_send_sigfox_message(uhfm_node_addr, &sigfox_message, &access_status);
	if (status != NODE_SUCCESS) goto errors;
errors:
	return status;
}

/*******************************************************************/
static NODE_status_t _MPMCM_send_mains_frequency_frame(NODE_address_t mpmcm_node_addr, NODE_address_t uhfm_node_addr) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	NODE_access_status_t access_status;
	XM_registers_list_t reg_list;
	NODE_dinfox_ul_payload_t dinfox_ul_payload;
	MPMCM_sigfox_ul_payload_mains_frequency_t sigfox_ul_payload_mains_frequency;
	UHFM_sigfox_message_t sigfox_message;
	uint8_t idx = 0;
	// Build registers list for mains frequency.
	reg_list.addr_list = (uint8_t*) MPMCM_REG_LIST_SIGFOX_UL_PAYLOAD_MAINS_FREQUENCY;
	reg_list.size = sizeof(MPMCM_REG_LIST_SIGFOX_UL_PAYLOAD_MAINS_FREQUENCY);
	// Read related registers.
	status = XM_read_registers(mpmcm_node_addr, &reg_list, (XM_node_registers_t*) &MPMCM_NODE_REGISTERS, &access_status);
	if (status != NODE_SUCCESS) goto errors;
	// Build mains frequency frame.
	sigfox_ul_payload_mains_frequency.f_min =  DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_MAINS_FREQUENCY_1], MPMCM_REG_MASK_MIN);
	sigfox_ul_payload_mains_frequency.f_mean = DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_MAINS_FREQUENCY_0], MPMCM_REG_MASK_MEAN);
	sigfox_ul_payload_mains_frequency.f_max =  DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_MAINS_FREQUENCY_1], MPMCM_REG_MASK_MAX);
	// Add header and copy payload
	dinfox_ul_payload.board_id = DINFOX_BOARD_ID_MPMCM;
	dinfox_ul_payload.node_addr = mpmcm_node_addr;
	for (idx=0 ; idx<MPMCM_SIGFOX_UL_PAYLOAD_MAINS_VOLTAGE_SIZE ; idx++) dinfox_ul_payload.node_data[idx] = sigfox_ul_payload_mains_frequency.frame[idx];
	// Set parameters.
	sigfox_message.ul_payload = (uint8_t*) dinfox_ul_payload.frame;
	sigfox_message.ul_payload_size = (NODE_DINFOX_PAYLOAD_HEADER_SIZE + MPMCM_SIGFOX_UL_PAYLOAD_MAINS_FREQUENCY_SIZE);
	sigfox_message.bidirectional_flag = 0;
	// Send message.
	status = UHFM_send_sigfox_message(uhfm_node_addr, &sigfox_message, &access_status);
	if (status != NODE_SUCCESS) goto errors;
errors:
	return status;
}

/*******************************************************************/
static NODE_status_t _MPMCM_send_mains_voltage_frame(NODE_address_t mpmcm_node_addr, NODE_address_t uhfm_node_addr, uint8_t channel_index) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	NODE_access_status_t access_status;
	XM_registers_list_t reg_list;
	NODE_dinfox_ul_payload_t dinfox_ul_payload;
	MPMCM_sigfox_ul_payload_mains_voltage_t sigfox_ul_payload_mains_voltage;
	UHFM_sigfox_message_t sigfox_message;
	uint8_t addr_list[sizeof(MPMCM_REG_LIST_SIGFOX_UL_PAYLOAD_MAINS_POWER)];
	uint32_t reg_offset = (channel_index * MPMCM_NUMBER_OF_REG_PER_DATA);
	uint8_t idx = 0;
	// Build registers list for mains voltage.
	for (idx=0 ; idx<sizeof(MPMCM_REG_LIST_SIGFOX_UL_PAYLOAD_MAINS_VOLTAGE) ; idx++) addr_list[idx] = MPMCM_REG_LIST_SIGFOX_UL_PAYLOAD_MAINS_VOLTAGE[idx] + reg_offset;
	reg_list.addr_list = (uint8_t*) addr_list;
	reg_list.size = sizeof(MPMCM_REG_LIST_SIGFOX_UL_PAYLOAD_MAINS_VOLTAGE);
	// Read related registers.
	status = XM_read_registers(mpmcm_node_addr, &reg_list, (XM_node_registers_t*) &MPMCM_NODE_REGISTERS, &access_status);
	if (status != NODE_SUCCESS) goto errors;
	// Build mains voltage frame.
	sigfox_ul_payload_mains_voltage.unused = 0;
	sigfox_ul_payload_mains_voltage.channel_index = channel_index;
	sigfox_ul_payload_mains_voltage.vrms_min =  DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_CH1_RMS_VOLTAGE_1 + reg_offset], MPMCM_REG_MASK_MIN);
	sigfox_ul_payload_mains_voltage.vrms_mean = DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_CH1_RMS_VOLTAGE_0 + reg_offset], MPMCM_REG_MASK_MEAN);
	sigfox_ul_payload_mains_voltage.vrms_max =  DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_CH1_RMS_VOLTAGE_1 + reg_offset], MPMCM_REG_MASK_MAX);
	// Add header and copy payload
	dinfox_ul_payload.board_id = DINFOX_BOARD_ID_MPMCM;
	dinfox_ul_payload.node_addr = mpmcm_node_addr;
	for (idx=0 ; idx<MPMCM_SIGFOX_UL_PAYLOAD_MAINS_VOLTAGE_SIZE ; idx++) dinfox_ul_payload.node_data[idx] = sigfox_ul_payload_mains_voltage.frame[idx];
	// Set parameters.
	sigfox_message.ul_payload = (uint8_t*) dinfox_ul_payload.frame;
	sigfox_message.ul_payload_size = (NODE_DINFOX_PAYLOAD_HEADER_SIZE + MPMCM_SIGFOX_UL_PAYLOAD_MAINS_VOLTAGE_SIZE);
	sigfox_message.bidirectional_flag = 0;
	// Send message.
	status = UHFM_send_sigfox_message(uhfm_node_addr, &sigfox_message, &access_status);
	if (status != NODE_SUCCESS) goto errors;
errors:
	return status;
}

/*******************************************************************/
static NODE_status_t _MPMCM_send_mains_power_frame(NODE_address_t mpmcm_node_addr, NODE_address_t uhfm_node_addr, uint8_t channel_index) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	NODE_access_status_t access_status;
	XM_registers_list_t reg_list;
	NODE_dinfox_ul_payload_t dinfox_ul_payload;
	MPMCM_sigfox_ul_payload_mains_power_t sigfox_ul_payload_mains_power;
	UHFM_sigfox_message_t sigfox_message;
	uint8_t addr_list[sizeof(MPMCM_REG_LIST_SIGFOX_UL_PAYLOAD_MAINS_POWER)];
	uint32_t reg_offset = (channel_index * MPMCM_NUMBER_OF_REG_PER_DATA);
	uint8_t idx = 0;
	// Build registers list for mains power frame.
	for (idx=0 ; idx<sizeof(MPMCM_REG_LIST_SIGFOX_UL_PAYLOAD_MAINS_POWER) ; idx++) addr_list[idx] = MPMCM_REG_LIST_SIGFOX_UL_PAYLOAD_MAINS_POWER[idx] + reg_offset;
	reg_list.addr_list = (uint8_t*) addr_list;
	reg_list.size = sizeof(MPMCM_REG_LIST_SIGFOX_UL_PAYLOAD_MAINS_POWER);
	// Read related registers.
	status = XM_read_registers(mpmcm_node_addr, &reg_list, (XM_node_registers_t*) &MPMCM_NODE_REGISTERS, &access_status);
	if (status != NODE_SUCCESS) goto errors;
	// Build mains power frame.
	sigfox_ul_payload_mains_power.unused = 0;
	sigfox_ul_payload_mains_power.channel_index = channel_index;
	sigfox_ul_payload_mains_power.pact_mean = DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_CH1_ACTIVE_POWER_0 +   reg_offset], MPMCM_REG_MASK_MEAN);
	sigfox_ul_payload_mains_power.pact_max =  DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_CH1_ACTIVE_POWER_1 +   reg_offset], MPMCM_REG_MASK_MAX);
	sigfox_ul_payload_mains_power.papp_mean = DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_CH1_APPARENT_POWER_0 + reg_offset], MPMCM_REG_MASK_MEAN);
	sigfox_ul_payload_mains_power.papp_max =  DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_CH1_APPARENT_POWER_1 + reg_offset], MPMCM_REG_MASK_MAX);
	// Add header and copy payload
	dinfox_ul_payload.board_id = DINFOX_BOARD_ID_MPMCM;
	dinfox_ul_payload.node_addr = mpmcm_node_addr;
	for (idx=0 ; idx<MPMCM_SIGFOX_UL_PAYLOAD_MAINS_POWER_SIZE ; idx++) dinfox_ul_payload.node_data[idx] = sigfox_ul_payload_mains_power.frame[idx];
	// Set parameters.
	sigfox_message.ul_payload = (uint8_t*) dinfox_ul_payload.frame;
	sigfox_message.ul_payload_size = (NODE_DINFOX_PAYLOAD_HEADER_SIZE + MPMCM_SIGFOX_UL_PAYLOAD_MAINS_POWER_SIZE);
	sigfox_message.bidirectional_flag = 0;
	// Send message.
	status = UHFM_send_sigfox_message(uhfm_node_addr, &sigfox_message, &access_status);
	if (status != NODE_SUCCESS) goto errors;
errors:
	return status;
}

/*******************************************************************/
static NODE_status_t _MPMCM_send_mains_power_factor_frame(NODE_address_t mpmcm_node_addr, NODE_address_t uhfm_node_addr, uint8_t channel_index) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	NODE_access_status_t access_status;
	XM_registers_list_t reg_list;
	NODE_dinfox_ul_payload_t dinfox_ul_payload;
	MPMCM_sigfox_ul_payload_mains_power_factor_t sigfox_ul_payload_mains_power_factor;
	UHFM_sigfox_message_t sigfox_message;
	uint8_t addr_list[sizeof(MPMCM_REG_LIST_SIGFOX_UL_PAYLOAD_MAINS_POWER_FACTOR)];
	uint32_t reg_offset = (channel_index * MPMCM_NUMBER_OF_REG_PER_DATA);
	uint8_t idx = 0;
	// Build registers list for power factor frame.
	for (idx=0 ; idx<sizeof(MPMCM_REG_LIST_SIGFOX_UL_PAYLOAD_MAINS_POWER_FACTOR) ; idx++) addr_list[idx] = MPMCM_REG_LIST_SIGFOX_UL_PAYLOAD_MAINS_POWER_FACTOR[idx] + reg_offset;
	reg_list.addr_list = (uint8_t*) addr_list;
	reg_list.size = sizeof(MPMCM_REG_LIST_SIGFOX_UL_PAYLOAD_MAINS_POWER_FACTOR);
	// Read related registers.
	status = XM_read_registers(mpmcm_node_addr, &reg_list, (XM_node_registers_t*) &MPMCM_NODE_REGISTERS, &access_status);
	if (status != NODE_SUCCESS) goto errors;
	// Build mains power factor frame.
	sigfox_ul_payload_mains_power_factor.unused = 0;
	sigfox_ul_payload_mains_power_factor.channel_index = channel_index;
	sigfox_ul_payload_mains_power_factor.pf_min =  DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_CH1_POWER_FACTOR_1 + reg_offset], MPMCM_REG_MASK_MIN);
	sigfox_ul_payload_mains_power_factor.pf_mean = DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_CH1_POWER_FACTOR_0 + reg_offset], MPMCM_REG_MASK_MEAN);
	sigfox_ul_payload_mains_power_factor.pf_max =  DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_CH1_POWER_FACTOR_1 + reg_offset], MPMCM_REG_MASK_MAX);
	// Add header and copy payload
	dinfox_ul_payload.board_id = DINFOX_BOARD_ID_MPMCM;
	dinfox_ul_payload.node_addr = mpmcm_node_addr;
	for (idx=0 ; idx<MPMCM_SIGFOX_UL_PAYLOAD_MAINS_POWER_FACTOR_SIZE ; idx++) dinfox_ul_payload.node_data[idx] = sigfox_ul_payload_mains_power_factor.frame[idx];
	// Set parameters.
	sigfox_message.ul_payload = (uint8_t*) dinfox_ul_payload.frame;
	sigfox_message.ul_payload_size = (NODE_DINFOX_PAYLOAD_HEADER_SIZE + MPMCM_SIGFOX_UL_PAYLOAD_MAINS_POWER_FACTOR_SIZE);
	sigfox_message.bidirectional_flag = 0;
	// Send message.
	status = UHFM_send_sigfox_message(uhfm_node_addr, &sigfox_message, &access_status);
	if (status != NODE_SUCCESS) goto errors;
errors:
	return status;
}

/*******************************************************************/
static NODE_status_t _MPMCM_send_mains_energy_frame(NODE_address_t mpmcm_node_addr, NODE_address_t uhfm_node_addr, uint8_t channel_index) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	NODE_access_status_t access_status;
	XM_registers_list_t reg_list;
	NODE_dinfox_ul_payload_t dinfox_ul_payload;
	MPMCM_sigfox_ul_payload_mains_energy_t sigfox_ul_payload_mains_energy;
	UHFM_sigfox_message_t sigfox_message;
	uint8_t addr_list[sizeof(MPMCM_REG_LIST_SIGFOX_UL_PAYLOAD_MAINS_ENERGY)];
	uint32_t reg_offset = (channel_index * MPMCM_NUMBER_OF_REG_PER_DATA);
	uint8_t idx = 0;
	// Build registers list for energy frame.
	for (idx=0 ; idx<sizeof(MPMCM_REG_LIST_SIGFOX_UL_PAYLOAD_MAINS_ENERGY) ; idx++) addr_list[idx] = MPMCM_REG_LIST_SIGFOX_UL_PAYLOAD_MAINS_ENERGY[idx] + reg_offset;
	reg_list.addr_list = (uint8_t*) addr_list;
	reg_list.size = sizeof(MPMCM_REG_LIST_SIGFOX_UL_PAYLOAD_MAINS_ENERGY);
	// Read related registers.
	status = XM_read_registers(mpmcm_node_addr, &reg_list, (XM_node_registers_t*) &MPMCM_NODE_REGISTERS, &access_status);
	if (status != NODE_SUCCESS) goto errors;
	// Build mains energy frame.
	sigfox_ul_payload_mains_energy.unused = 0;
	sigfox_ul_payload_mains_energy.channel_index = channel_index;
	sigfox_ul_payload_mains_energy.eact = DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_CH1_ENERGY + reg_offset], MPMCM_REG_MASK_ACTIVE_ENERGY);
	sigfox_ul_payload_mains_energy.eapp = DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_CH1_ENERGY + reg_offset], MPMCM_REG_MASK_APPARENT_ENERGY);
	// Add header and copy payload
	dinfox_ul_payload.board_id = DINFOX_BOARD_ID_MPMCM;
	dinfox_ul_payload.node_addr = mpmcm_node_addr;
	for (idx=0 ; idx<MPMCM_SIGFOX_UL_PAYLOAD_MAINS_ENERGY_SIZE ; idx++) dinfox_ul_payload.node_data[idx] = sigfox_ul_payload_mains_energy.frame[idx];
	// Set parameters.
	sigfox_message.ul_payload = (uint8_t*) dinfox_ul_payload.frame;
	sigfox_message.ul_payload_size = (NODE_DINFOX_PAYLOAD_HEADER_SIZE + MPMCM_SIGFOX_UL_PAYLOAD_MAINS_ENERGY_SIZE);
	sigfox_message.bidirectional_flag = 0;
	// Send message.
	status = UHFM_send_sigfox_message(uhfm_node_addr, &sigfox_message, &access_status);
	if (status != NODE_SUCCESS) goto errors;
errors:
	return status;
}

/*** MPMCM functions ***/

/*******************************************************************/
NODE_status_t MPMCM_write_line_data(NODE_line_data_write_t* line_data_write, NODE_access_status_t* write_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	// Check parameters.
	if ((line_data_write == NULL) || (write_status == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Check index.
	if ((line_data_write -> line_data_index) >= MPMCM_LINE_DATA_INDEX_LAST) {
		status = NODE_ERROR_LINE_DATA_INDEX;
		goto errors;
	}
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
errors:
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
		status = XM_read_register((line_data_read -> node_addr), reg_addr, (XM_node_registers_t*) &MPMCM_NODE_REGISTERS, read_status);
		if ((status != NODE_SUCCESS) || ((read_status -> flags) != 0)) goto errors;
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
		case MPMCM_LINE_DATA_INDEX_TIC_PACT:
		case MPMCM_LINE_DATA_INDEX_TIC_PAPP:
			// Check error value.
			if (field_value != DINFOX_ELECTRICAL_POWER_ERROR_VALUE) {
				// Convert to 5 digits string.
				string_status = STRING_value_to_5_digits_string(DINFOX_get_mw_mva(field_value), (char_t*) field_str);
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
	// Check event driven payloads.
	status = COMMON_check_event_driven_payloads(node_ul_payload, &node_reg);
	if (status != NODE_SUCCESS) goto errors;
errors:
	return status;
}

/*******************************************************************/
NODE_status_t MPMCM_radio_process(NODE_address_t mpmcm_node_addr, NODE_address_t uhfm_node_addr) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	NODE_access_status_t access_status;
	uint8_t ame = 0;
	uint8_t lte = 0;
	uint8_t ltm = 0;
	uint8_t mvd = 0;
	uint8_t ticd = 0;
	uint32_t reg_control_1 = 0;
	uint8_t channel_idx = 0;
	uint8_t mains_voltage_chx_sent = 0;
	// Store accumulated data of all channels (synchronization reset in case of POR).
	reg_control_1 |= MPMCM_REG_CONTROL_1_MASK_CH1S;
	reg_control_1 |= MPMCM_REG_CONTROL_1_MASK_CH2S;
	reg_control_1 |= MPMCM_REG_CONTROL_1_MASK_CH3S;
	reg_control_1 |= MPMCM_REG_CONTROL_1_MASK_CH4S;
	reg_control_1 |= MPMCM_REG_CONTROL_1_MASK_TICS;
	reg_control_1 |= MPMCM_REG_CONTROL_1_MASK_FRQS;
	status = XM_write_register(mpmcm_node_addr, MPMCM_REG_ADDR_CONTROL_1, reg_control_1, reg_control_1, AT_BUS_DEFAULT_TIMEOUT_MS, &access_status);
	if (status != NODE_SUCCESS) goto errors;
	// Do not send frames on POR.
	if (mpmcm_por_flag != 0) goto errors;
	// Send status frame.
	status = _MPMCM_send_status_frame(mpmcm_node_addr, uhfm_node_addr);
	if (status != NODE_SUCCESS) goto errors;
	// Read node configuration.
	status = XM_read_register(mpmcm_node_addr, MPMCM_REG_ADDR_CONFIGURATION_0, (XM_node_registers_t*) &MPMCM_NODE_REGISTERS, &access_status);
	if ((status != NODE_SUCCESS) || (access_status.flags != 0)) goto errors;
	// Update local flags.
	ame =  DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_CONFIGURATION_0], MPMCM_REG_CONFIGURATION_0_MASK_AME);
	lte =  DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_CONFIGURATION_0], MPMCM_REG_CONFIGURATION_0_MASK_LTE);
	ltm =  DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_CONFIGURATION_0], MPMCM_REG_CONFIGURATION_0_MASK_LTM);
	mvd =  DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_STATUS_1], MPMCM_REG_STATUS_1_MASK_MVD);
	ticd = DINFOX_read_field(MPMCM_REGISTERS[MPMCM_REG_ADDR_STATUS_1], MPMCM_REG_STATUS_1_MASK_TICD);
	// Check MVD flag.
	if (mvd != 0) {
		// Send frequency frame.
		status = _MPMCM_send_mains_frequency_frame(mpmcm_node_addr, uhfm_node_addr);
		if (status != NODE_SUCCESS) goto errors;
	}
	// Channels loop.
	for (channel_idx=0 ; channel_idx<(MPMCM_NUMBER_OF_ACI_CHANNELS + 1) ; channel_idx++) {
		// Do not send any other frame if mains voltage was not present 2 consecutive times.
		if ((channel_idx < MPMCM_NUMBER_OF_ACI_CHANNELS) && (((mvd == 0) && (mpmcm_mvd_flag == 0)) || (ame == 0))) continue;
		// Do not send any frame if TIC was not detected 2 consecutive times.
		if ((channel_idx == MPMCM_TIC_CHANNEL_INDEX) && (((ticd == 0) && (mpmcm_ticd_flag == 0)) || (lte == 0))) continue;
		// Mains voltage.
		if (channel_idx < MPMCM_NUMBER_OF_ACI_CHANNELS) {
			// Check if voltage has already been sent from another channel.
			if (mains_voltage_chx_sent == 0) {
				// Send voltage frame.
				status = _MPMCM_send_mains_voltage_frame(mpmcm_node_addr, uhfm_node_addr, channel_idx);
				if (status != NODE_SUCCESS) goto errors;
				// Update flag.
				mains_voltage_chx_sent = 1;
			}
		}
		else {
			// Mains voltage is only available only TIC standard interface.
			if (ltm != 0) {
				// Send voltage frame.
				status = _MPMCM_send_mains_voltage_frame(mpmcm_node_addr, uhfm_node_addr, channel_idx);
				if (status != NODE_SUCCESS) goto errors;
			}
		}
		// Do not send any other frame if current sensors is not connected.
		if ((channel_idx < MPMCM_NUMBER_OF_ACI_CHANNELS) && ((MPMCM_REGISTERS[MPMCM_REG_ADDR_STATUS_1] & (0b1 << channel_idx)) == 0)) continue;
		// Mains power.
		status = _MPMCM_send_mains_power_frame(mpmcm_node_addr, uhfm_node_addr, channel_idx);
		if (status != NODE_SUCCESS) goto errors;
		// Mains power factor.
		if ((channel_idx < MPMCM_NUMBER_OF_ACI_CHANNELS) || (ltm != 0)) {
			// Send power factor frame.
			status = _MPMCM_send_mains_power_factor_frame(mpmcm_node_addr, uhfm_node_addr, channel_idx);
			if (status != NODE_SUCCESS) goto errors;
		}
		// Mains energy.
		status = _MPMCM_send_mains_energy_frame(mpmcm_node_addr, uhfm_node_addr, channel_idx);
		if (status != NODE_SUCCESS) goto errors;
	}
errors:
	// Clear POR flag and update MVD and TICD flags.
	mpmcm_por_flag = 0;
	mpmcm_mvd_flag = mvd;
	mpmcm_ticd_flag = ticd;
	return status;
}
