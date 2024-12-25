/*
 * radio_mpmcm.c
 *
 *  Created on: 26 feb. 2023
 *      Author: Ludo
 */

#include "radio_mpmcm.h"

#include "common_registers.h"
#include "mpmcm_registers.h"
#include "radio.h"
#include "radio_common.h"
#include "radio_uhfm.h"
#include "swreg.h"
#include "uhfm_registers.h"
#include "una.h"

/*** RADIO MPMCM local macros ***/

#define RADIO_MPMCM_UL_PAYLOAD_STATUS_SIZE                  1
#define RADIO_MPMCM_UL_PAYLOAD_MAINS_POWER_FACTOR_SIZE      4
#define RADIO_MPMCM_UL_PAYLOAD_MAINS_ENERGY_SIZE            5
#define RADIO_MPMCM_UL_PAYLOAD_MAINS_FREQUENCY_SIZE         6
#define RADIO_MPMCM_UL_PAYLOAD_MAINS_VOLTAGE_SIZE           7
#define RADIO_MPMCM_UL_PAYLOAD_MAINS_POWER_SIZE             9

/*** RADIO MPMCM local structures ***/

/*******************************************************************/
typedef union {
	uint8_t frame[RADIO_MPMCM_UL_PAYLOAD_STATUS_SIZE];
	struct {
		unsigned unused : 2;
		unsigned mvd : 1;
		unsigned ticd : 1;
		unsigned ch4d : 1;
		unsigned ch3d : 1;
		unsigned ch2d : 1;
		unsigned ch1d : 1;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} RADIO_MPMCM_ul_payload_status_t;

/*******************************************************************/
typedef union {
	uint8_t frame[RADIO_MPMCM_UL_PAYLOAD_MAINS_VOLTAGE_SIZE];
	struct {
		unsigned unused : 5;
		unsigned channel_index : 3;
		unsigned vrms_min : 16;
		unsigned vrms_mean : 16;
		unsigned vrms_max : 16;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} RADIO_MPMCM_ul_payload_mains_voltage_t;

/*******************************************************************/
typedef union {
	uint8_t frame[RADIO_MPMCM_UL_PAYLOAD_MAINS_FREQUENCY_SIZE];
	struct {
		unsigned f_min : 16;
		unsigned f_mean : 16;
		unsigned f_max : 16;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} RADIO_MPMCM_ul_payload_mains_frequency_t;

/*******************************************************************/
typedef union {
	uint8_t frame[RADIO_MPMCM_UL_PAYLOAD_MAINS_POWER_SIZE];
	struct {
		unsigned unused : 5;
		unsigned channel_index : 3;
		unsigned pact_mean : 16;
		unsigned pact_max : 16;
		unsigned papp_mean : 16;
		unsigned papp_max : 16;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} RADIO_MPMCM_ul_payload_mains_power_t;

/*******************************************************************/
typedef union {
	uint8_t frame[RADIO_MPMCM_UL_PAYLOAD_MAINS_POWER_FACTOR_SIZE];
	struct {
		unsigned unused : 5;
		unsigned channel_index : 3;
		unsigned pf_min : 8;
		unsigned pf_mean : 8;
		unsigned pf_max : 8;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} RADIO_MPMCM_ul_payload_mains_power_factor_t;

/*******************************************************************/
typedef union {
	uint8_t frame[RADIO_MPMCM_UL_PAYLOAD_MAINS_ENERGY_SIZE];
	struct {
		unsigned unused : 5;
		unsigned channel_index : 3;
		unsigned eact : 16;
		unsigned eapp : 16;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} RADIO_MPMCM_ul_payload_mains_energy_t;

/*******************************************************************/
typedef union {
    struct {
        unsigned por : 1;
        unsigned mvd : 1;
        unsigned ticd : 1;
    };
    uint8_t all;
} RADIO_MPMCM_flags_t;

/*******************************************************************/
typedef struct {
    RADIO_MPMCM_flags_t flags;
    uint32_t registers[MPMCM_REGISTER_ADDRESS_LAST];
} RADIO_MPMCM_context_t;

/*** MPMCM local global variables ***/

static const uint8_t RADIO_MPMCM_REGISTERS_STATUS[] = {
    MPMCM_REGISTER_ADDRESS_STATUS_1,
};

static const uint8_t RADIO_MPMCM_REGISTERS_MAINS_VOLTAGE[] = {
	MPMCM_REGISTER_ADDRESS_STATUS_1,
	MPMCM_REGISTER_ADDRESS_CH1_RMS_VOLTAGE_0,
	MPMCM_REGISTER_ADDRESS_CH1_RMS_VOLTAGE_1
};

static const uint8_t RADIO_MPMCM_REGISTERS_MAINS_FREQUENCY[] = {
	MPMCM_REGISTER_ADDRESS_MAINS_FREQUENCY_0,
	MPMCM_REGISTER_ADDRESS_MAINS_FREQUENCY_1,
};

static const uint8_t RADIO_MPMCM_REGISTERS_MAINS_POWER[] = {
	MPMCM_REGISTER_ADDRESS_CH1_ACTIVE_POWER_0,
	MPMCM_REGISTER_ADDRESS_CH1_ACTIVE_POWER_1,
	MPMCM_REGISTER_ADDRESS_CH1_APPARENT_POWER_0,
	MPMCM_REGISTER_ADDRESS_CH1_APPARENT_POWER_1
};

static const uint8_t RADIO_MPMCM_REGISTERS_MAINS_POWER_FACTOR[] = {
	MPMCM_REGISTER_ADDRESS_CH1_POWER_FACTOR_0,
	MPMCM_REGISTER_ADDRESS_CH1_POWER_FACTOR_1,
};

static const uint8_t RADIO_MPMCM_REGISTERS_MAINS_ENERGY[] = {
	MPMCM_REGISTER_ADDRESS_CH1_ENERGY,
};

static RADIO_MPMCM_context_t radio_mpmcm_ctx = {
    .flags.por = 1,
    .flags.mvd = 0,
    .flags.ticd = 0,
};

/*** MPMCM local functions ***/

/*******************************************************************/
static RADIO_status_t _RADIO_MPMCM_build_ul_node_payload_status(UNA_node_t* mpmcm_node, RADIO_ul_node_payload_t* node_payload) {
	// Local variables.
	RADIO_status_t status = RADIO_SUCCESS;
	NODE_status_t node_status = NODE_SUCCESS;
	UNA_access_status_t access_status;
	RADIO_MPMCM_ul_payload_status_t ul_payload_status;
	uint8_t idx = 0;
	// Read related registers.
	node_status = NODE_read_registers(mpmcm_node, (uint8_t*) RADIO_MPMCM_REGISTERS_STATUS, sizeof(RADIO_MPMCM_REGISTERS_STATUS), (uint32_t*) (radio_mpmcm_ctx.registers), &access_status);
	NODE_exit_error(RADIO_ERROR_BASE_NODE);
	if (access_status.flags != 0) goto errors;
	// Build status frame.
	ul_payload_status.frame[0] = (uint8_t) (radio_mpmcm_ctx.registers[MPMCM_REGISTER_ADDRESS_STATUS_1] & 0x3F);
	// Copy payload.
    for (idx=0 ; idx<RADIO_MPMCM_UL_PAYLOAD_STATUS_SIZE ; idx++) {
        (node_payload->payload)[idx] = ul_payload_status.frame[idx];
    }
    node_payload->payload_size = RADIO_MPMCM_UL_PAYLOAD_STATUS_SIZE;
errors:
	return status;
}

/*******************************************************************/
static RADIO_status_t _RADIO_MPMCM_build_ul_node_payload_frequency(UNA_node_t* mpmcm_node, RADIO_ul_node_payload_t* node_payload) {
	// Local variables.
	RADIO_status_t status = RADIO_SUCCESS;
	NODE_status_t node_status = NODE_SUCCESS;
	UNA_access_status_t access_status;
	RADIO_MPMCM_ul_payload_mains_frequency_t ul_payload_mains_frequency;
	uint8_t idx = 0;
	// Read related registers.
	node_status = NODE_read_registers(mpmcm_node, (uint8_t*) RADIO_MPMCM_REGISTERS_MAINS_FREQUENCY, sizeof(RADIO_MPMCM_REGISTERS_MAINS_FREQUENCY), (uint32_t*) (radio_mpmcm_ctx.registers), &access_status);
	NODE_exit_error(RADIO_ERROR_BASE_NODE);
	if (access_status.flags != 0) goto errors;
	// Build mains frequency frame.
	ul_payload_mains_frequency.f_min =  SWREG_read_field(radio_mpmcm_ctx.registers[MPMCM_REGISTER_ADDRESS_MAINS_FREQUENCY_1], MPMCM_REGISTER_MASK_MIN);
	ul_payload_mains_frequency.f_mean = SWREG_read_field(radio_mpmcm_ctx.registers[MPMCM_REGISTER_ADDRESS_MAINS_FREQUENCY_0], MPMCM_REGISTER_MASK_MEAN);
	ul_payload_mains_frequency.f_max =  SWREG_read_field(radio_mpmcm_ctx.registers[MPMCM_REGISTER_ADDRESS_MAINS_FREQUENCY_1], MPMCM_REGISTER_MASK_MAX);
	// Copy payload.
    for (idx=0 ; idx<RADIO_MPMCM_UL_PAYLOAD_MAINS_FREQUENCY_SIZE ; idx++) {
        (node_payload->payload)[idx] = ul_payload_mains_frequency.frame[idx];
    }
    node_payload->payload_size = RADIO_MPMCM_UL_PAYLOAD_MAINS_FREQUENCY_SIZE;
errors:
	return status;
}

/*******************************************************************/
static RADIO_status_t _RADIO_MPMCM_build_ul_node_payload_voltage(UNA_node_t* mpmcm_node, RADIO_ul_node_payload_t* node_payload, uint8_t channel_index) {
	// Local variables.
	RADIO_status_t status = RADIO_SUCCESS;
	NODE_status_t node_status = NODE_SUCCESS;
	UNA_access_status_t access_status;
	RADIO_MPMCM_ul_payload_mains_voltage_t ul_payload_mains_voltage;
	uint8_t reg_addr_list[sizeof(RADIO_MPMCM_REGISTERS_MAINS_POWER)];
	uint8_t reg_offset = (channel_index * MPMCM_NUMBER_OF_REGISTERS_PER_DATA);
	uint8_t idx = 0;
	// Build registers list for mains voltage.
	for (idx = 0; idx < sizeof(RADIO_MPMCM_REGISTERS_MAINS_VOLTAGE); idx++) {
	    reg_addr_list[idx] = RADIO_MPMCM_REGISTERS_MAINS_VOLTAGE[idx] + reg_offset;
	}
	// Read related registers.
	node_status = NODE_read_registers(mpmcm_node, (uint8_t*) reg_addr_list, sizeof(reg_addr_list), (uint32_t*) (radio_mpmcm_ctx.registers), &access_status);
	NODE_exit_error(RADIO_ERROR_BASE_NODE);
    if (access_status.flags != 0) goto errors;
	// Build mains voltage frame.
	ul_payload_mains_voltage.unused = 0;
	ul_payload_mains_voltage.channel_index = channel_index;
	ul_payload_mains_voltage.vrms_min =  SWREG_read_field(radio_mpmcm_ctx.registers[MPMCM_REGISTER_ADDRESS_CH1_RMS_VOLTAGE_1 + reg_offset], MPMCM_REGISTER_MASK_MIN);
	ul_payload_mains_voltage.vrms_mean = SWREG_read_field(radio_mpmcm_ctx.registers[MPMCM_REGISTER_ADDRESS_CH1_RMS_VOLTAGE_0 + reg_offset], MPMCM_REGISTER_MASK_MEAN);
	ul_payload_mains_voltage.vrms_max =  SWREG_read_field(radio_mpmcm_ctx.registers[MPMCM_REGISTER_ADDRESS_CH1_RMS_VOLTAGE_1 + reg_offset], MPMCM_REGISTER_MASK_MAX);
	// Copy payload.
    for (idx=0 ; idx<RADIO_MPMCM_UL_PAYLOAD_MAINS_VOLTAGE_SIZE ; idx++) {
        (node_payload->payload)[idx] = ul_payload_mains_voltage.frame[idx];
    }
    node_payload->payload_size = RADIO_MPMCM_UL_PAYLOAD_MAINS_VOLTAGE_SIZE;
errors:
	return status;
}

/*******************************************************************/
static RADIO_status_t _RADIO_MPMCM_build_ul_node_payload_power(UNA_node_t* mpmcm_node, RADIO_ul_node_payload_t* node_payload, uint8_t channel_index) {
	// Local variables.
	RADIO_status_t status = RADIO_SUCCESS;
	NODE_status_t node_status = NODE_SUCCESS;
	UNA_access_status_t access_status;
	RADIO_MPMCM_ul_payload_mains_power_t ul_payload_mains_power;
	uint8_t reg_addr_list[sizeof(RADIO_MPMCM_REGISTERS_MAINS_POWER)];
	uint8_t reg_offset = (channel_index * MPMCM_NUMBER_OF_REGISTERS_PER_DATA);
	uint8_t idx = 0;
	// Build registers list for mains power frame.
	for (idx=0 ; idx<sizeof(RADIO_MPMCM_REGISTERS_MAINS_POWER) ; idx++) {
	    reg_addr_list[idx] = RADIO_MPMCM_REGISTERS_MAINS_POWER[idx] + reg_offset;
	}
	// Read related registers.
	node_status = NODE_read_registers(mpmcm_node, (uint8_t*) reg_addr_list, sizeof(reg_addr_list), (uint32_t*) (radio_mpmcm_ctx.registers), &access_status);
    NODE_exit_error(RADIO_ERROR_BASE_NODE);
    if (access_status.flags != 0) goto errors;
	// Build mains power frame.
	ul_payload_mains_power.unused = 0;
	ul_payload_mains_power.channel_index = channel_index;
	ul_payload_mains_power.pact_mean = SWREG_read_field(radio_mpmcm_ctx.registers[MPMCM_REGISTER_ADDRESS_CH1_ACTIVE_POWER_0 +   reg_offset], MPMCM_REGISTER_MASK_MEAN);
	ul_payload_mains_power.pact_max =  SWREG_read_field(radio_mpmcm_ctx.registers[MPMCM_REGISTER_ADDRESS_CH1_ACTIVE_POWER_1 +   reg_offset], MPMCM_REGISTER_MASK_MAX);
	ul_payload_mains_power.papp_mean = SWREG_read_field(radio_mpmcm_ctx.registers[MPMCM_REGISTER_ADDRESS_CH1_APPARENT_POWER_0 + reg_offset], MPMCM_REGISTER_MASK_MEAN);
	ul_payload_mains_power.papp_max =  SWREG_read_field(radio_mpmcm_ctx.registers[MPMCM_REGISTER_ADDRESS_CH1_APPARENT_POWER_1 + reg_offset], MPMCM_REGISTER_MASK_MAX);
	// Copy payload.
    for (idx=0 ; idx<RADIO_MPMCM_UL_PAYLOAD_MAINS_POWER_SIZE ; idx++) {
        (node_payload->payload)[idx] = ul_payload_mains_power.frame[idx];
    }
    node_payload->payload_size = RADIO_MPMCM_UL_PAYLOAD_MAINS_POWER_SIZE;
errors:
	return status;
}

/*******************************************************************/
static RADIO_status_t _RADIO_MPMCM_build_ul_node_payload_power_factor(UNA_node_t* mpmcm_node, RADIO_ul_node_payload_t* node_payload, uint8_t channel_index) {
	// Local variables.
	RADIO_status_t status = RADIO_SUCCESS;
	NODE_status_t node_status = NODE_SUCCESS;
	UNA_access_status_t access_status;
	RADIO_MPMCM_ul_payload_mains_power_factor_t ul_payload_mains_power_factor;
	uint8_t reg_addr_list[sizeof(RADIO_MPMCM_REGISTERS_MAINS_POWER_FACTOR)];
	uint8_t reg_offset = (channel_index * MPMCM_NUMBER_OF_REGISTERS_PER_DATA);
	uint8_t idx = 0;
	// Build registers list for power factor frame.
	for (idx=0 ; idx<sizeof(RADIO_MPMCM_REGISTERS_MAINS_POWER_FACTOR) ; idx++) {
	    reg_addr_list[idx] = RADIO_MPMCM_REGISTERS_MAINS_POWER_FACTOR[idx] + reg_offset;
	}
	// Read related registers.
	node_status = NODE_read_registers(mpmcm_node, (uint8_t*) reg_addr_list, sizeof(reg_addr_list), (uint32_t*) (radio_mpmcm_ctx.registers), &access_status);
    NODE_exit_error(RADIO_ERROR_BASE_NODE);
    if (access_status.flags != 0) goto errors;
	// Build mains power factor frame.
	ul_payload_mains_power_factor.unused = 0;
	ul_payload_mains_power_factor.channel_index = channel_index;
	ul_payload_mains_power_factor.pf_min =  SWREG_read_field(radio_mpmcm_ctx.registers[MPMCM_REGISTER_ADDRESS_CH1_POWER_FACTOR_1 + reg_offset], MPMCM_REGISTER_MASK_MIN);
	ul_payload_mains_power_factor.pf_mean = SWREG_read_field(radio_mpmcm_ctx.registers[MPMCM_REGISTER_ADDRESS_CH1_POWER_FACTOR_0 + reg_offset], MPMCM_REGISTER_MASK_MEAN);
	ul_payload_mains_power_factor.pf_max =  SWREG_read_field(radio_mpmcm_ctx.registers[MPMCM_REGISTER_ADDRESS_CH1_POWER_FACTOR_1 + reg_offset], MPMCM_REGISTER_MASK_MAX);
	// Copy payload.
    for (idx=0 ; idx<RADIO_MPMCM_UL_PAYLOAD_MAINS_POWER_FACTOR_SIZE ; idx++) {
        (node_payload->payload)[idx] = ul_payload_mains_power_factor.frame[idx];
    }
    node_payload->payload_size = RADIO_MPMCM_UL_PAYLOAD_MAINS_POWER_FACTOR_SIZE;
errors:
	return status;
}

/*******************************************************************/
static RADIO_status_t _RADIO_MPMCM_build_ul_node_payload_energy(UNA_node_t* mpmcm_node, RADIO_ul_node_payload_t* node_payload, uint8_t channel_index) {
	// Local variables.
	RADIO_status_t status = RADIO_SUCCESS;
	NODE_status_t node_status = NODE_SUCCESS;
	UNA_access_status_t access_status;
	RADIO_MPMCM_ul_payload_mains_energy_t ul_payload_mains_energy;
	uint8_t reg_addr_list[sizeof(RADIO_MPMCM_REGISTERS_MAINS_ENERGY)];
	uint8_t reg_offset = (channel_index * MPMCM_NUMBER_OF_REGISTERS_PER_DATA);
	uint8_t idx = 0;
	// Build registers list for energy frame.
	for (idx=0 ; idx<sizeof(RADIO_MPMCM_REGISTERS_MAINS_ENERGY) ; idx++) {
	    reg_addr_list[idx] = RADIO_MPMCM_REGISTERS_MAINS_ENERGY[idx] + reg_offset;
	}
	// Read related registers.
	node_status = NODE_read_registers(mpmcm_node, (uint8_t*) reg_addr_list, sizeof(reg_addr_list), (uint32_t*) (radio_mpmcm_ctx.registers), &access_status);
    NODE_exit_error(RADIO_ERROR_BASE_NODE);
    if (access_status.flags != 0) goto errors;
	// Build mains energy frame.
	ul_payload_mains_energy.unused = 0;
	ul_payload_mains_energy.channel_index = channel_index;
	ul_payload_mains_energy.eact = SWREG_read_field(radio_mpmcm_ctx.registers[MPMCM_REGISTER_ADDRESS_CH1_ENERGY + reg_offset], MPMCM_REGISTER_MASK_ACTIVE_ENERGY);
	ul_payload_mains_energy.eapp = SWREG_read_field(radio_mpmcm_ctx.registers[MPMCM_REGISTER_ADDRESS_CH1_ENERGY + reg_offset], MPMCM_REGISTER_MASK_APPARENT_ENERGY);
	// Copy payload.
    for (idx=0 ; idx<RADIO_MPMCM_UL_PAYLOAD_MAINS_ENERGY_SIZE ; idx++) {
        (node_payload->payload)[idx] = ul_payload_mains_energy.frame[idx];
    }
    node_payload->payload_size = RADIO_MPMCM_UL_PAYLOAD_MAINS_ENERGY_SIZE;
errors:
	return status;
}

/*** RADIO MPMCM functions ***/

/*******************************************************************/
RADIO_status_t RADIO_MPMCM_build_ul_node_payload(RADIO_ul_node_payload_t* node_payload) {
	// Local variables.
	RADIO_status_t status = RADIO_SUCCESS;
	uint8_t idx = 0;
	// Reset registers.
    for (idx = 0; idx < MPMCM_REGISTER_ADDRESS_LAST; idx++) {
        radio_mpmcm_ctx.registers[idx] = MPMCM_REGISTER_ERROR_VALUE[idx];
    }
	// Check event driven payloads.
	status = RADIO_COMMON_check_event_driven_payloads(node_payload, (uint32_t*) radio_mpmcm_ctx.registers);
	if (status != RADIO_SUCCESS) goto errors;
errors:
	return status;
}

/*******************************************************************/
RADIO_status_t RADIO_MPMCM_process(UNA_node_t* mpmcm_node, RADIO_MPMCM_radio_transmit_t radio_transmit_pfn) {
	// Local variables.
	RADIO_status_t status = RADIO_SUCCESS;
	NODE_status_t node_status = NODE_SUCCESS;
	UNA_access_status_t access_status;
	RADIO_ul_node_payload_t node_payload;
	uint8_t node_payload_bytes[UHFM_UL_PAYLOAD_MAX_SIZE_BYTES];
	uint8_t ame = 0;
	uint8_t lte = 0;
	uint8_t ltm = 0;
	uint8_t mvd = 0;
	uint8_t ticd = 0;
	uint32_t reg_control_1 = 0;
	uint8_t channel_idx = 0;
	uint8_t mains_voltage_chx_sent = 0;
	uint8_t idx = 0;
    // Check parameters.
    if ((mpmcm_node == NULL) || (radio_transmit_pfn == NULL)) {
        status = RADIO_ERROR_NULL_PARAMETER;
        goto errors;
    }
    // Reset registers.
    for (idx = 0; idx < MPMCM_REGISTER_ADDRESS_LAST; idx++) {
        radio_mpmcm_ctx.registers[idx] = MPMCM_REGISTER_ERROR_VALUE[idx];
    }
    // Build node payload structure.
    node_payload.node = mpmcm_node;
    node_payload.payload = (uint8_t*) node_payload_bytes;
    node_payload.payload_size = 0;
    node_payload.payload_type_counter = 0;
	// Store accumulated data of all channels (synchronization reset in case of POR).
	reg_control_1 |= MPMCM_REGISTER_CONTROL_1_MASK_CH1S;
	reg_control_1 |= MPMCM_REGISTER_CONTROL_1_MASK_CH2S;
	reg_control_1 |= MPMCM_REGISTER_CONTROL_1_MASK_CH3S;
	reg_control_1 |= MPMCM_REGISTER_CONTROL_1_MASK_CH4S;
	reg_control_1 |= MPMCM_REGISTER_CONTROL_1_MASK_TICS;
	reg_control_1 |= MPMCM_REGISTER_CONTROL_1_MASK_FRQS;
	node_status = NODE_write_register(mpmcm_node, MPMCM_REGISTER_ADDRESS_CONTROL_1, reg_control_1, reg_control_1, &access_status);
	NODE_exit_error(RADIO_ERROR_BASE_NODE);
	// Do not send frames on POR.
	if (radio_mpmcm_ctx.flags.por != 0) goto errors;
	// Send status frame.
	status = _RADIO_MPMCM_build_ul_node_payload_status(mpmcm_node, &node_payload);
	if (status != RADIO_SUCCESS) goto errors;
	status = radio_transmit_pfn(&node_payload, 0);
	if (status != RADIO_SUCCESS) goto errors;
	// Read node configuration.
	node_status = NODE_read_register(mpmcm_node, MPMCM_REGISTER_ADDRESS_CONFIGURATION_0, &(radio_mpmcm_ctx.registers[MPMCM_REGISTER_ADDRESS_CONFIGURATION_0]), &access_status);
	NODE_exit_error(RADIO_ERROR_BASE_NODE);
	if (access_status.flags != 0) goto errors;
	// Update local flags.
	ame =  (uint8_t) SWREG_read_field(radio_mpmcm_ctx.registers[MPMCM_REGISTER_ADDRESS_CONFIGURATION_0], MPMCM_REGISTER_CONFIGURATION_0_MASK_AME);
	lte =  (uint8_t) SWREG_read_field(radio_mpmcm_ctx.registers[MPMCM_REGISTER_ADDRESS_CONFIGURATION_0], MPMCM_REGISTER_CONFIGURATION_0_MASK_LTE);
	ltm =  (uint8_t) SWREG_read_field(radio_mpmcm_ctx.registers[MPMCM_REGISTER_ADDRESS_CONFIGURATION_0], MPMCM_REGISTER_CONFIGURATION_0_MASK_LTM);
	mvd =  (uint8_t) SWREG_read_field(radio_mpmcm_ctx.registers[MPMCM_REGISTER_ADDRESS_STATUS_1], MPMCM_REGISTER_STATUS_1_MASK_MVD);
	ticd = (uint8_t) SWREG_read_field(radio_mpmcm_ctx.registers[MPMCM_REGISTER_ADDRESS_STATUS_1], MPMCM_REGISTER_STATUS_1_MASK_TICD);
	// Check MVD flag.
	if (mvd != 0) {
		// Send frequency frame.
		status = _RADIO_MPMCM_build_ul_node_payload_frequency(mpmcm_node, &node_payload);
		if (status != RADIO_SUCCESS) goto errors;
		status = radio_transmit_pfn(&node_payload, 0);
        if (status != RADIO_SUCCESS) goto errors;
	}
	// Channels loop.
	for (channel_idx = 0; channel_idx < MPMCM_CHANNEL_INDEX_LAST; channel_idx++) {
		// Do not send any other frame if mains voltage was not present 2 consecutive times.
		if ((channel_idx <= MPMCM_CHANNEL_INDEX_ACI3) && (((mvd == 0) && (radio_mpmcm_ctx.flags.mvd == 0)) || (ame == 0))) continue;
		// Do not send any frame if TIC was not detected 2 consecutive times.
		if ((channel_idx == MPMCM_CHANNEL_INDEX_TIC) && (((ticd == 0) && (radio_mpmcm_ctx.flags.ticd == 0)) || (lte == 0))) continue;
		// Mains voltage.
		if (channel_idx <= MPMCM_CHANNEL_INDEX_ACI3) {
			// Check if voltage has already been sent from another channel.
			if (mains_voltage_chx_sent == 0) {
				// Send voltage frame.
				status = _RADIO_MPMCM_build_ul_node_payload_voltage(mpmcm_node, &node_payload, channel_idx);
				if (status != RADIO_SUCCESS) goto errors;
				status = radio_transmit_pfn(&node_payload, 0);
                if (status != RADIO_SUCCESS) goto errors;
				// Update flag.
				mains_voltage_chx_sent = 1;
			}
		}
		else {
			// Mains voltage is only available only TIC standard interface.
			if (ltm != 0) {
				// Send voltage frame.
				status = _RADIO_MPMCM_build_ul_node_payload_voltage(mpmcm_node, &node_payload, channel_idx);
				if (status != RADIO_SUCCESS) goto errors;
				status = radio_transmit_pfn(&node_payload, 0);
                if (status != RADIO_SUCCESS) goto errors;
			}
		}
		// Do not send any other frame if current sensors is not connected.
		if ((channel_idx <= MPMCM_CHANNEL_INDEX_ACI3) && ((radio_mpmcm_ctx.registers[MPMCM_REGISTER_ADDRESS_STATUS_1] & (0b1 << channel_idx)) == 0)) continue;
		// Mains power.
		status = _RADIO_MPMCM_build_ul_node_payload_power(mpmcm_node, &node_payload, channel_idx);
		if (status != RADIO_SUCCESS) goto errors;
		status = radio_transmit_pfn(&node_payload, 0);
        if (status != RADIO_SUCCESS) goto errors;
		// Mains power factor.
		if ((channel_idx <= MPMCM_CHANNEL_INDEX_ACI3) || (ltm != 0)) {
			// Send power factor frame.
			status = _RADIO_MPMCM_build_ul_node_payload_power_factor(mpmcm_node, &node_payload, channel_idx);
			if (status != RADIO_SUCCESS) goto errors;
			status = radio_transmit_pfn(&node_payload, 0);
            if (status != RADIO_SUCCESS) goto errors;
		}
		// Mains energy.
		status = _RADIO_MPMCM_build_ul_node_payload_energy(mpmcm_node, &node_payload, channel_idx);
		if (status != RADIO_SUCCESS) goto errors;
		status = radio_transmit_pfn(&node_payload, 0);
        if (status != RADIO_SUCCESS) goto errors;
	}
errors:
	// Clear POR flag and update MVD and TICD flags.
	radio_mpmcm_ctx.flags.por = 0;
	radio_mpmcm_ctx.flags.mvd = mvd;
	radio_mpmcm_ctx.flags.ticd = ticd;
	return status;
}
