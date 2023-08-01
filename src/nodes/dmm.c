/*
 * dmm.c
 *
 *  Created on: 04 mar. 2023
 *      Author: Ludo
 */

#include "dmm.h"

#include "at_bus.h"
#include "dmm_reg.h"
#include "common.h"
#include "dinfox.h"
#include "error.h"
#include "gpio.h"
#include "i2c.h"
#include "mapping.h"
#include "node.h"
#include "pwr.h"
#include "rcc_reg.h"
#include "string.h"
#include "version.h"
#include "xm.h"

/*** DMM local macros ***/

#define DMM_SIGFOX_PAYLOAD_MONITORING_SIZE		5

#define DMM_SIGFOX_PAYLOAD_LOOP_MAX				10

#define NODE_SIGFOX_UL_PERIOD_SECONDS_MIN		60
#define NODE_SIGFOX_UL_PERIOD_SECONDS_DEFAULT	300

#define NODE_SIGFOX_DL_PERIOD_SECONDS_MIN		600
#define NODE_SIGFOX_DL_PERIOD_SECONDS_DEFAULT	21600

/*** DMM local structures ***/

typedef enum {
	DMM_SIGFOX_PAYLOAD_TYPE_STARTUP = 0,
	DMM_SIGFOX_PAYLOAD_TYPE_MONITORING,
	DMM_SIGFOX_PAYLOAD_TYPE_ERROR_STACK,
	DMM_SIGFOX_PAYLOAD_TYPE_LAST
} DMM_sigfox_payload_type_t;

typedef union {
	uint8_t frame[DMM_SIGFOX_PAYLOAD_MONITORING_SIZE];
	struct {
		unsigned vrs : 16;
		unsigned vhmi : 16;
		unsigned nodes_count : 8;
	} __attribute__((scalar_storage_order("big-endian"))) __attribute__((packed));
} DMM_sigfox_payload_monitoring_t;

/*** DMM local global variables ***/

static uint32_t DMM_INTERNAL_REGISTERS[DMM_REG_ADDR_LAST];
static uint32_t DMM_REGISTERS[DMM_REG_ADDR_LAST];

static const NODE_line_data_t DMM_LINE_DATA[DMM_LINE_DATA_INDEX_LAST - COMMON_LINE_DATA_INDEX_LAST] = {
	{"VRS =", " V", STRING_FORMAT_DECIMAL, 0, DMM_REG_ADDR_ANALOG_DATA_1, DMM_REG_ANALOG_DATA_1_MASK_VRS},
	{"VHMI =", " V", STRING_FORMAT_DECIMAL, 0, DMM_REG_ADDR_ANALOG_DATA_1, DMM_REG_ANALOG_DATA_1_MASK_VHMI},
	{"VUSB =", " V", STRING_FORMAT_DECIMAL, 0, DMM_REG_ADDR_ANALOG_DATA_2, DMM_REG_ANALOG_DATA_2_MASK_VUSB},
	{"NODES_CNT =", STRING_NULL, STRING_FORMAT_DECIMAL, 0, DMM_REG_ADDR_STATUS_CONTROL_1, DMM_REG_STATUS_CONTROL_1_MASK_NODES_COUNT},
	{"UL_PRD =", " s", STRING_FORMAT_DECIMAL, 0, DMM_REG_ADDR_SYSTEM_CONFIGURATION, DMM_REG_SYSTEM_CONFIGURATION_MASK_UL_PERIOD},
	{"DL_PRD =", " s", STRING_FORMAT_DECIMAL, 0, DMM_REG_ADDR_SYSTEM_CONFIGURATION, DMM_REG_SYSTEM_CONFIGURATION_MASK_DL_PERIOD}
};

static const uint32_t DMM_REG_ERROR_VALUE[DMM_REG_ADDR_LAST] = {
	COMMON_REG_ERROR_VALUE
	0x00000000,
	((DINFOX_VOLTAGE_ERROR_VALUE << 16) | (DINFOX_VOLTAGE_ERROR_VALUE << 0)),
	(DINFOX_VOLTAGE_ERROR_VALUE << 0),
	((DINFOX_TIME_ERROR_VALUE << 8) | (DINFOX_TIME_ERROR_VALUE << 0))
};

static const uint8_t DMM_REG_LIST_SIGFOX_PAYLOAD_MONITORING[] = {
	DMM_REG_ADDR_STATUS_CONTROL_1,
	DMM_REG_ADDR_ANALOG_DATA_1
};

static const DINFOX_register_access_t DMM_REG_ACCESS[DMM_REG_ADDR_LAST] = {
	COMMON_REG_ACCESS
	DINFOX_REG_ACCESS_READ_WRITE,
	DINFOX_REG_ACCESS_READ_ONLY,
	DINFOX_REG_ACCESS_READ_ONLY
};

static const DMM_sigfox_payload_type_t DMM_SIGFOX_PAYLOAD_PATTERN[] = {
	DMM_SIGFOX_PAYLOAD_TYPE_STARTUP,
	DMM_SIGFOX_PAYLOAD_TYPE_MONITORING,
	DMM_SIGFOX_PAYLOAD_TYPE_MONITORING,
	DMM_SIGFOX_PAYLOAD_TYPE_MONITORING,
	DMM_SIGFOX_PAYLOAD_TYPE_MONITORING,
	DMM_SIGFOX_PAYLOAD_TYPE_MONITORING,
	DMM_SIGFOX_PAYLOAD_TYPE_MONITORING,
	DMM_SIGFOX_PAYLOAD_TYPE_MONITORING,
	DMM_SIGFOX_PAYLOAD_TYPE_MONITORING,
	DMM_SIGFOX_PAYLOAD_TYPE_MONITORING,
	DMM_SIGFOX_PAYLOAD_TYPE_MONITORING,
	DMM_SIGFOX_PAYLOAD_TYPE_ERROR_STACK
};

/*** DMM local functions ***/

/* RESET DMM ANALOG DATA.
 * @param:	None.
 * @return:	None.
 */
void _DMM_reset_analog_data(void) {
	// Local variables.
	uint32_t unused_mask = 0;
	// Reset to error value.
	DINFOX_write_field(&(DMM_INTERNAL_REGISTERS[COMMON_REG_ADDR_ANALOG_DATA_0]), &unused_mask, DINFOX_VOLTAGE_ERROR_VALUE, COMMON_REG_ANALOG_DATA_0_MASK_VMCU);
	DINFOX_write_field(&(DMM_INTERNAL_REGISTERS[COMMON_REG_ADDR_ANALOG_DATA_0]), &unused_mask, DINFOX_TEMPERATURE_ERROR_VALUE, COMMON_REG_ANALOG_DATA_0_MASK_TMCU);
	DINFOX_write_field(&(DMM_INTERNAL_REGISTERS[DMM_REG_ADDR_ANALOG_DATA_1]), &unused_mask, DINFOX_VOLTAGE_ERROR_VALUE, DMM_REG_ANALOG_DATA_1_MASK_VRS);
	DINFOX_write_field(&(DMM_INTERNAL_REGISTERS[DMM_REG_ADDR_ANALOG_DATA_1]), &unused_mask, DINFOX_VOLTAGE_ERROR_VALUE, DMM_REG_ANALOG_DATA_1_MASK_VHMI);
	DINFOX_write_field(&(DMM_INTERNAL_REGISTERS[DMM_REG_ADDR_ANALOG_DATA_2]), &unused_mask, DINFOX_VOLTAGE_ERROR_VALUE, DMM_REG_ANALOG_DATA_2_MASK_VUSB);
}

/*** DMM functions ***/

/* INIT DMM REGISTERS.
 * @param:	None.
 * @return:	None.
 */
void DMM_init_registers(void) {
	// Local variables.
	uint8_t idx = 0;
	uint32_t unused_mask = 0;
	// Reset all registers.
	for (idx=0 ; idx<DMM_REG_ADDR_LAST ; idx++) {
		DMM_INTERNAL_REGISTERS[idx] = 0;
	}
	// Node ID register.
	DINFOX_write_field(&(DMM_INTERNAL_REGISTERS[COMMON_REG_ADDR_NODE_ID]), &unused_mask, DINFOX_NODE_ADDRESS_DMM, COMMON_REG_NODE_ID_MASK_NODE_ADDR);
	DINFOX_write_field(COMMON_REG_ADDR_NODE_ID, &unused_mask, DINFOX_BOARD_ID_DMM, COMMON_REG_NODE_ID_MASK_BOARD_ID);
	// HW version register.
#ifdef HW1_0
	DINFOX_write_field(&(DMM_INTERNAL_REGISTERS[COMMON_REG_ADDR_HW_VERSION]), &unused_mask, 1, COMMON_REG_HW_VERSION_MASK_MAJOR);
	DINFOX_write_field(&(DMM_INTERNAL_REGISTERS[COMMON_REG_ADDR_HW_VERSION]), &unused_mask, 0, COMMON_REG_HW_VERSION_MASK_MINOR);
#endif
	// SW version register 0.
	DINFOX_write_field(&(DMM_INTERNAL_REGISTERS[COMMON_REG_ADDR_SW_VERSION_0]), &unused_mask, GIT_MAJOR_VERSION, COMMON_REG_SW_VERSION_0_MASK_MAJOR);
	DINFOX_write_field(&(DMM_INTERNAL_REGISTERS[COMMON_REG_ADDR_SW_VERSION_0]), &unused_mask, GIT_MINOR_VERSION, COMMON_REG_SW_VERSION_0_MASK_MINOR);
	DINFOX_write_field(&(DMM_INTERNAL_REGISTERS[COMMON_REG_ADDR_SW_VERSION_0]), &unused_mask, GIT_COMMIT_INDEX, COMMON_REG_SW_VERSION_0_MASK_COMMIT_INDEX);
	DINFOX_write_field(&(DMM_INTERNAL_REGISTERS[COMMON_REG_ADDR_SW_VERSION_0]), &unused_mask, GIT_DIRTY_FLAG, COMMON_REG_SW_VERSION_0_MASK_DTYF);
	// SW version register 1.
	DINFOX_write_field(&(DMM_INTERNAL_REGISTERS[COMMON_REG_ADDR_SW_VERSION_1]), &unused_mask, GIT_COMMIT_ID, COMMON_REG_SW_VERSION_1_MASK_COMMIT_ID);
	// Reset flags registers.
	DINFOX_write_field(&(DMM_INTERNAL_REGISTERS[COMMON_REG_ADDR_RESET_FLAGS]), &unused_mask, ((uint32_t) (((RCC -> CSR) >> 24) & 0xFF)), COMMON_REG_RESET_FLAGS_MASK_ALL);
	// Load default values.
	_DMM_reset_analog_data();
	DINFOX_write_field(&(DMM_INTERNAL_REGISTERS[DMM_REG_ADDR_SYSTEM_CONFIGURATION]), &unused_mask, DINFOX_convert_seconds(NODE_SIGFOX_UL_PERIOD_SECONDS_DEFAULT), DMM_REG_SYSTEM_CONFIGURATION_MASK_UL_PERIOD);
	DINFOX_write_field(&(DMM_INTERNAL_REGISTERS[DMM_REG_ADDR_SYSTEM_CONFIGURATION]), &unused_mask, DINFOX_convert_seconds(NODE_SIGFOX_DL_PERIOD_SECONDS_DEFAULT), DMM_REG_SYSTEM_CONFIGURATION_MASK_DL_PERIOD);
}

/* WRITE DMM REGISTER.
 * @param write_params:		Writing operation parameters.
 * @param reg_value:		Value to write in register.
 * @param reg_mask:			Write operation mask.
 * @param write_status:		Pointer to the writing operation status.
 * @return status:			Function execution status.
 */
NODE_status_t DMM_write_register(NODE_access_parameters_t* write_params, uint32_t reg_value, uint32_t reg_mask, NODE_access_status_t* write_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	ADC_status_t adc1_status = ADC_SUCCESS;
	I2C_status_t i2c1_status = I2C_SUCCESS;
	uint32_t adc_data = 0;
	int8_t tmcu_degrees = 0;
	uint32_t temp = 0;
	uint8_t hmi_on = 0;
	uint32_t unused_mask = 0;
	// Reset write status.
	(*write_status).all = 0;
	// Check parameters.
	if ((write_params == NULL) || (write_status == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Check address.
	if ((write_params -> reg_addr) >= DMM_REG_ADDR_LAST) {
		status = NODE_ERROR_REGISTER_ADDRESS;
		goto errors;
	}
	// Check access.
	if (DMM_REG_ACCESS[(write_params -> reg_addr)] == DINFOX_REG_ACCESS_READ_ONLY) {
		status = NODE_ERROR_REGISTER_READ_ONLY;
		goto errors;
	}
	// Read register.
	temp = DMM_INTERNAL_REGISTERS[(write_params -> reg_addr)];
	// Compute new value.
	temp &= ~reg_mask;
	temp |= (reg_value & reg_mask);
	// Write register.
	DMM_INTERNAL_REGISTERS[(write_params -> reg_addr)] = temp;
	// Check actions.
	if (DINFOX_read_field(DMM_INTERNAL_REGISTERS[COMMON_REG_ADDR_STATUS_CONTROL_0], COMMON_REG_STATUS_CONTROL_0_MASK_RTRG) != 0) {
		// Clear flag.
		DINFOX_write_field(&(DMM_INTERNAL_REGISTERS[COMMON_REG_ADDR_STATUS_CONTROL_0]), &unused_mask, 0, COMMON_REG_STATUS_CONTROL_0_MASK_RTRG);
		// Reset MCU.
		PWR_software_reset();
	}
	// Measure trigger bit.
	if (DINFOX_read_field(DMM_INTERNAL_REGISTERS[COMMON_REG_ADDR_STATUS_CONTROL_0], COMMON_REG_STATUS_CONTROL_0_MASK_MTRG) != 0) {
		// Clear flag.
		DINFOX_write_field(&(DMM_INTERNAL_REGISTERS[COMMON_REG_ADDR_STATUS_CONTROL_0]), &unused_mask, 0, COMMON_REG_STATUS_CONTROL_0_MASK_MTRG);
		// Reset results.
		_DMM_reset_analog_data();
		// Check HMI power status.
		hmi_on = GPIO_read(&GPIO_HMI_POWER_ENABLE);
		if (hmi_on == 0) {
			// Turn HMI on.
			i2c1_status = I2C1_power_on();
			I2C1_check_status(NODE_ERROR_BASE_I2C);
		}
		// Perform analog measurements.
		adc1_status = ADC1_perform_measurements();
		// Disable HMI power supply.
		if (hmi_on == 0) {
			I2C1_power_off();
		}
		ADC1_check_status(NODE_ERROR_BASE_ADC);
		// VMCU.
		adc1_status = ADC1_get_data(ADC_DATA_INDEX_VMCU_MV, &adc_data);
		ADC1_check_status(NODE_ERROR_BASE_ADC);
		DINFOX_write_field(&(DMM_INTERNAL_REGISTERS[COMMON_REG_ADDR_ANALOG_DATA_0]), &unused_mask, DINFOX_convert_mv(adc_data), COMMON_REG_ANALOG_DATA_0_MASK_VMCU);
		// TMCU.
		adc1_status = ADC1_get_tmcu(&tmcu_degrees);
		ADC1_check_status(NODE_ERROR_BASE_ADC);
		DINFOX_write_field(&(DMM_INTERNAL_REGISTERS[COMMON_REG_ADDR_ANALOG_DATA_0]), &unused_mask, DINFOX_convert_degrees(tmcu_degrees), COMMON_REG_ANALOG_DATA_0_MASK_TMCU);
		// VRS.
		adc1_status = ADC1_get_data(ADC_DATA_INDEX_VRS_MV, &adc_data);
		ADC1_check_status(NODE_ERROR_BASE_ADC);
		DINFOX_write_field(&(DMM_INTERNAL_REGISTERS[DMM_REG_ADDR_ANALOG_DATA_1]), &unused_mask, DINFOX_convert_mv(adc_data), DMM_REG_ANALOG_DATA_1_MASK_VRS);
		// VHMI.
		adc1_status = ADC1_get_data(ADC_DATA_INDEX_VHMI_MV, &adc_data);
		ADC1_check_status(NODE_ERROR_BASE_ADC);
		DINFOX_write_field(&(DMM_INTERNAL_REGISTERS[DMM_REG_ADDR_ANALOG_DATA_1]), &unused_mask, DINFOX_convert_mv(adc_data), DMM_REG_ANALOG_DATA_1_MASK_VHMI);
		// VHMI.
		adc1_status = ADC1_get_data(ADC_DATA_INDEX_VUSB_MV, &adc_data);
		ADC1_check_status(NODE_ERROR_BASE_ADC);
		DINFOX_write_field(&(DMM_INTERNAL_REGISTERS[DMM_REG_ADDR_ANALOG_DATA_2]), &unused_mask, DINFOX_convert_mv(adc_data), DMM_REG_ANALOG_DATA_2_MASK_VUSB);
	}
errors:
	return status;
}

/* READ DMM REGISTER.
 * @param read_params:	Pointer to the read operation parameters.
 * @param reg_value:	Pointer to the register value.
 * @param read_status:	Pointer to the read operation status.
 * @return status:		Function execution status.
 */
NODE_status_t DMM_read_register(NODE_access_parameters_t* read_params, uint32_t* reg_value, NODE_access_status_t* read_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	uint32_t unused_mask = 0;
	// Reset read status.
	(*read_status).all = 0;
	// Check parameters.
	if ((read_params == NULL) || (read_status == NULL) || (reg_value == NULL)) {
		status = NODE_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Check address.
	if ((read_params -> reg_addr) >= DMM_REG_ADDR_LAST) {
		status = NODE_ERROR_REGISTER_ADDRESS;
		goto errors;
	}
	// Update required registers.
	switch (read_params -> reg_addr) {
	case COMMON_REG_ADDR_ERROR_STACK:
		// Unstack error.
		DINFOX_write_field(&(DMM_INTERNAL_REGISTERS[COMMON_REG_ADDR_ERROR_STACK]), &unused_mask, (uint32_t) (ERROR_stack_read()), COMMON_REG_ERROR_STACK_MASK_ERROR);
		break;
	case DMM_REG_ADDR_STATUS_CONTROL_1:
		DINFOX_write_field(&(DMM_INTERNAL_REGISTERS[DMM_REG_ADDR_STATUS_CONTROL_1]), &unused_mask, (uint32_t) (NODES_LIST.count), DMM_REG_STATUS_CONTROL_1_MASK_NODES_COUNT);
		break;
	default:
		// Nothing to do on other registers.
		break;
	}
	// Read register.
	(*reg_value) = DMM_INTERNAL_REGISTERS[(read_params -> reg_addr)];
errors:
	return status;
}

/* WRITE DMM DATA.
 * @param line_data_write:	Pointer to the data write structure.
 * @param read_status:		Pointer to the writing operation status.
 * @return status:			Function execution status.
 */
NODE_status_t DMM_write_line_data(NODE_line_data_write_t* line_data_write, NODE_access_status_t* write_status) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	// Call common function with local data.
	status = XM_write_line_data(line_data_write, (NODE_line_data_t*) DMM_LINE_DATA, (uint32_t*) DMM_REG_WRITE_TIMEOUT_MS, write_status);
	return status;
}

/* READ DMM DATA.
 * @param line_data_read:	Pointer to the data read structure.
 * @param read_status:		Pointer to the reading operation status.
 * @return status:			Function execution status.
 */
NODE_status_t DMM_read_line_data(NODE_line_data_read_t* line_data_read, NODE_access_status_t* read_status) {
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
	if ((line_data_read -> line_data_index) >= DMM_LINE_DATA_INDEX_LAST) {
		status = NODE_ERROR_LINE_DATA_INDEX;
		goto errors;
	}
	// Build node registers structure.
	node_reg.value = (uint32_t*) DMM_REGISTERS;
	node_reg.error = (uint32_t*) DMM_REG_ERROR_VALUE;
	// Check common range.
	if ((line_data_read -> line_data_index) < COMMON_LINE_DATA_INDEX_LAST) {
		// Call common function.
		status = COMMON_read_line_data(line_data_read, &node_reg);
		if (status != NODE_SUCCESS) goto errors;
	}
	else {
		// Compute specific string data index and register address.
		str_data_idx = ((line_data_read -> line_data_index) - COMMON_LINE_DATA_INDEX_LAST);
		reg_addr = DMM_LINE_DATA[str_data_idx].reg_addr;
		// Add data name.
		NODE_append_name_string((char_t*) DMM_LINE_DATA[str_data_idx].name);
		buffer_size = 0;
		// Reset result to error.
		NODE_flush_string_value();
		NODE_append_value_string((char_t*) NODE_ERROR_STRING);
		// Update register.
		status = XM_read_register((line_data_read -> node_addr), reg_addr, DMM_REG_ERROR_VALUE[reg_addr], &(DMM_REGISTERS[reg_addr]), read_status);
		if ((status != NODE_SUCCESS) || ((read_status -> all) != 0)) goto errors;
		// Compute field.
		field_value = DINFOX_read_field(DMM_REGISTERS[reg_addr], DMM_LINE_DATA[str_data_idx].field_mask);
		// Check index.
		switch (line_data_read -> line_data_index) {
		case DMM_LINE_DATA_INDEX_VRS:
		case DMM_LINE_DATA_INDEX_VHMI:
		case DMM_LINE_DATA_INDEX_VUSB:
			// Check error value.
			if (field_value != DINFOX_VOLTAGE_ERROR_VALUE) {
				// Convert to 5 digits string.
				string_status = STRING_value_to_5_digits_string(DINFOX_get_mv(field_value), (char_t*) field_str);
				STRING_check_status(NODE_ERROR_BASE_STRING);
				// Add string.
				NODE_flush_string_value();
				NODE_append_value_string(field_str);
				// Add unit.
				NODE_append_value_string((char_t*) DMM_LINE_DATA[str_data_idx].unit);
			}
			break;
		case DMM_LINE_DATA_INDEX_NODES_COUNT:
			// Raw value.
			NODE_flush_string_value();
			NODE_append_value_int32(field_value,  DMM_LINE_DATA[str_data_idx].print_format,  DMM_LINE_DATA[str_data_idx].print_prefix);
			// Add unit.
			NODE_append_value_string((char_t*) DMM_LINE_DATA[str_data_idx].unit);
			break;
		case DMM_LINE_DATA_INDEX_UL_PERIOD:
		case DMM_LINE_DATA_INDEX_DL_PERIOD:
			// Convert to seconds.
			NODE_flush_string_value();
			NODE_append_value_int32(DINFOX_get_seconds(field_value),  DMM_LINE_DATA[str_data_idx].print_format,  DMM_LINE_DATA[str_data_idx].print_prefix);
			// Add unit.
			NODE_append_value_string((char_t*) DMM_LINE_DATA[str_data_idx].unit);
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

/* UPDATE DMM NODE SIGFOX UPLINK PAYLOAD.
 * @param ul_payload_update:	Pointer to the UL payload update structure.
 * @return status:				Function execution status.
 */
NODE_status_t DMM_build_sigfox_ul_payload(NODE_ul_payload_update_t* ul_payload_update) {
	// Local variables.
	NODE_status_t status = NODE_SUCCESS;
	NODE_access_status_t write_status;
	XM_node_registers_t node_reg;
	XM_registers_list_t reg_list;
	DMM_sigfox_payload_monitoring_t sigfox_payload_monitoring;
	uint8_t idx = 0;
	uint32_t loop_count = 0;
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
	node_reg.value = (uint32_t*) DMM_REGISTERS;
	node_reg.error = (uint32_t*) DMM_REG_ERROR_VALUE;
	// Reset payload size.
	(*(ul_payload_update -> size)) = 0;
	// Main loop.
	do {
		// Check payload type.
		switch (DMM_SIGFOX_PAYLOAD_PATTERN[ul_payload_update -> node -> radio_transmission_count]) {
		case DMM_SIGFOX_PAYLOAD_TYPE_STARTUP:
			// Check flag.
			if ((ul_payload_update -> node -> startup_data_sent) == 0) {
				// Use common format.
				status = COMMON_build_sigfox_payload_startup(ul_payload_update, &node_reg);
				if (status != NODE_SUCCESS) goto errors;
				// Update flag.
				(ul_payload_update -> node -> startup_data_sent) = 1;
			}
			break;
		case DMM_SIGFOX_PAYLOAD_TYPE_ERROR_STACK:
			// Use common format.
			status = COMMON_build_sigfox_payload_error_stack(ul_payload_update, &node_reg);
			if (status != NODE_SUCCESS) goto errors;
			break;
		case DMM_SIGFOX_PAYLOAD_TYPE_MONITORING:
			// Build registers list.
			reg_list.addr_list = (uint8_t*) DMM_REG_LIST_SIGFOX_PAYLOAD_MONITORING;
			reg_list.size = sizeof(DMM_REG_LIST_SIGFOX_PAYLOAD_MONITORING);
			// Reset registers.
			status = XM_reset_registers(&reg_list, &node_reg);
			if (status != NODE_SUCCESS) goto errors;
			// Perform measurements.
			status = XM_perform_measurements((ul_payload_update -> node -> address), &write_status);
			if (status != NODE_SUCCESS) goto errors;
			// Check write status.
			if (write_status.all == 0) {
				// Read related registers.
				status = XM_read_registers((ul_payload_update -> node -> address), &reg_list, &node_reg);
				if (status != NODE_SUCCESS) goto errors;
			}
			// Build monitoring payload.
			sigfox_payload_monitoring.vrs = DINFOX_read_field(DMM_REGISTERS[DMM_REG_ADDR_ANALOG_DATA_1], DMM_REG_ANALOG_DATA_1_MASK_VRS);
			sigfox_payload_monitoring.vhmi = DINFOX_read_field(DMM_REGISTERS[DMM_REG_ADDR_ANALOG_DATA_1], DMM_REG_ANALOG_DATA_1_MASK_VHMI);
			sigfox_payload_monitoring.nodes_count = DINFOX_read_field(DMM_REGISTERS[DMM_REG_ADDR_STATUS_CONTROL_1], DMM_REG_STATUS_CONTROL_1_MASK_NODES_COUNT);
			// Copy payload.
			for (idx=0 ; idx<DMM_SIGFOX_PAYLOAD_MONITORING_SIZE ; idx++) {
				(ul_payload_update -> ul_payload)[idx] = sigfox_payload_monitoring.frame[idx];
			}
			(*(ul_payload_update -> size)) = DMM_SIGFOX_PAYLOAD_MONITORING_SIZE;
			break;
		default:
			status = NODE_ERROR_SIGFOX_PAYLOAD_TYPE;
			goto errors;
		}
		// Increment transmission count.
		(ul_payload_update -> node -> radio_transmission_count) = ((ul_payload_update -> node -> radio_transmission_count) + 1) % (sizeof(DMM_SIGFOX_PAYLOAD_PATTERN));
		// Exit in case of loop error.
		loop_count++;
		if (loop_count > DMM_SIGFOX_PAYLOAD_LOOP_MAX) {
			status = NODE_ERROR_SIGFOX_PAYLOAD_LOOP;
			goto errors;
		}
	}
	while ((*(ul_payload_update -> size)) == 0);
errors:
	return status;
}
