/*
 * una_dmm.c
 *
 *  Created on: 04 mar. 2023
 *      Author: Ludo
 */

#include "una_dmm.h"

#include "dmm_flags.h"
#include "dmm_registers.h"
#include "common_registers.h"
#include "error.h"
#include "error_base.h"
#include "node.h"
#include "nvm.h"
#include "nvm_address.h"
#include "power.h"
#include "pwr.h"
#include "rcc_registers.h"
#include "swreg.h"
#include "version.h"
#include "una.h"

/*** UNA DMM local macros ***/

#define UNA_DMM_RADIO_UL_PERIOD_SECONDS_MIN     60
#define UNA_DMM_RADIO_DL_PERIOD_SECONDS_MIN     600
#define UNA_DMM_NODE_SCAN_PERIOD_SECONDS_MIN    3600

/*** UNA DMM local global variables ***/

static uint32_t UNA_DMM_REGISTERS[DMM_REGISTER_ADDRESS_LAST];

/*** UNA DMM local functions ***/

/*******************************************************************/
static void _UNA_DMM_write_nvm_register(uint8_t reg_addr, uint32_t reg_value) {
    // Local variables.
    NVM_status_t nvm_status = NVM_SUCCESS;
    uint8_t nvm_byte = 0;
    uint8_t idx = 0;
    // Byte loop.
    for (idx = 0; idx < UNA_REGISTER_SIZE_BYTES; idx++) {
        // Compute byte.
        nvm_byte = (uint8_t) (((reg_value) >> (idx << 3)) & 0x000000FF);
        // Write NVM.
        nvm_status = NVM_write_byte((NVM_ADDRESS_UNA_REGISTERS + (reg_addr << 2) + idx), nvm_byte);
        NVM_stack_error(ERROR_BASE_NVM);
        if (nvm_status != NVM_SUCCESS) break;
    }
}

/*******************************************************************/
static uint32_t _UNA_DMM_read_nvm_register(uint8_t reg_addr) {
    // Local variables.
    uint32_t reg_value = 0;
    NVM_status_t nvm_status = NVM_SUCCESS;
    uint8_t nvm_byte = 0;
    uint8_t idx = 0;
    // Byte loop.
    for (idx=0 ; idx<UNA_REGISTER_SIZE_BYTES ; idx++) {
        // Read NVM.
        nvm_status = NVM_read_byte((NVM_ADDRESS_UNA_REGISTERS + (reg_addr << 2) + idx), &nvm_byte);
        NVM_stack_error(ERROR_BASE_NVM);
        if (nvm_status != NVM_SUCCESS) {
            reg_value = 0;
            break;
        }
        reg_value |= ((uint32_t) nvm_byte) << (idx << 3);
    }
    return reg_value;
}

/*******************************************************************/
static void _UNA_DMM_load_dynamic_configuration(void) {
	// Local variables.
	uint8_t reg_addr = 0;
	uint32_t reg_value = 0;
	// Load configuration registers from NVM.
	for (reg_addr=DMM_REGISTER_ADDRESS_CONFIGURATION_0 ; reg_addr<DMM_REGISTER_ADDRESS_STATUS_1 ; reg_addr++) {
		// Read NVM.
		reg_value = _UNA_DMM_read_nvm_register(reg_addr);
		// Write register.
		UNA_DMM_REGISTERS[reg_addr] = reg_value;
	}
}

/*******************************************************************/
static void _UNA_DMM_reset_analog_data(void) {
	// Local variables.
	uint32_t unused_mask = 0;
	// Reset to error value.
	SWREG_write_field(&(UNA_DMM_REGISTERS[COMMON_REGISTER_ADDRESS_ANALOG_DATA_0]), &unused_mask, UNA_VOLTAGE_ERROR_VALUE, COMMON_REGISTER_ANALOG_DATA_0_MASK_VMCU);
	SWREG_write_field(&(UNA_DMM_REGISTERS[COMMON_REGISTER_ADDRESS_ANALOG_DATA_0]), &unused_mask, UNA_TEMPERATURE_ERROR_VALUE, COMMON_REGISTER_ANALOG_DATA_0_MASK_TMCU);
	SWREG_write_field(&(UNA_DMM_REGISTERS[DMM_REGISTER_ADDRESS_ANALOG_DATA_1]), &unused_mask, UNA_VOLTAGE_ERROR_VALUE, DMM_REGISTER_ANALOG_DATA_1_MASK_VRS);
	SWREG_write_field(&(UNA_DMM_REGISTERS[DMM_REGISTER_ADDRESS_ANALOG_DATA_1]), &unused_mask, UNA_VOLTAGE_ERROR_VALUE, DMM_REGISTER_ANALOG_DATA_1_MASK_VHMI);
	SWREG_write_field(&(UNA_DMM_REGISTERS[DMM_REGISTER_ADDRESS_ANALOG_DATA_2]), &unused_mask, UNA_VOLTAGE_ERROR_VALUE, DMM_REGISTER_ANALOG_DATA_2_MASK_VUSB);
}

/*******************************************************************/
static UNA_DMM_status_t _UNA_DMM_mtrg_callback(void) {
	// Local variables.
	UNA_DMM_status_t status = UNA_DMM_SUCCESS;
	ANALOG_status_t analog_status = ANALOG_SUCCESS;
	int32_t adc_data = 0;
	uint32_t unused_mask = 0;
	// Reset results.
	_UNA_DMM_reset_analog_data();
	// Turn ADC and HMI on.
	POWER_enable(POWER_REQUESTER_ID_DMM, POWER_DOMAIN_HMI, LPTIM_DELAY_MODE_STOP);
	POWER_enable(POWER_REQUESTER_ID_DMM, POWER_DOMAIN_ANALOG, LPTIM_DELAY_MODE_ACTIVE);
	// VMCU.
	analog_status = ANALOG_convert_channel(ANALOG_CHANNEL_VMCU_MV, &adc_data);
	ANALOG_exit_error(NODE_ERROR_BASE_ANALOG);
	SWREG_write_field(&(UNA_DMM_REGISTERS[COMMON_REGISTER_ADDRESS_ANALOG_DATA_0]), &unused_mask, UNA_convert_mv(adc_data), COMMON_REGISTER_ANALOG_DATA_0_MASK_VMCU);
	// TMCU.
	analog_status = ANALOG_convert_channel(ANALOG_CHANNEL_TMCU_DEGREES, &adc_data);
	ANALOG_exit_error(NODE_ERROR_BASE_ANALOG);
	SWREG_write_field(&(UNA_DMM_REGISTERS[COMMON_REGISTER_ADDRESS_ANALOG_DATA_0]), &unused_mask, UNA_convert_degrees(adc_data), COMMON_REGISTER_ANALOG_DATA_0_MASK_TMCU);
	// VRS.
	analog_status = ANALOG_convert_channel(ANALOG_CHANNEL_VRS_MV, &adc_data);
	ANALOG_exit_error(NODE_ERROR_BASE_ANALOG);
	SWREG_write_field(&(UNA_DMM_REGISTERS[DMM_REGISTER_ADDRESS_ANALOG_DATA_1]), &unused_mask, UNA_convert_mv(adc_data), DMM_REGISTER_ANALOG_DATA_1_MASK_VRS);
	// VHMI.
	analog_status = ANALOG_convert_channel(ANALOG_CHANNEL_VHMI_MV, &adc_data);
	ANALOG_exit_error(NODE_ERROR_BASE_ANALOG);
	SWREG_write_field(&(UNA_DMM_REGISTERS[DMM_REGISTER_ADDRESS_ANALOG_DATA_1]), &unused_mask, UNA_convert_mv(adc_data), DMM_REGISTER_ANALOG_DATA_1_MASK_VHMI);
	// VUSB.
	analog_status = ANALOG_convert_channel(ANALOG_CHANNEL_VUSB_MV, &adc_data);
	ANALOG_exit_error(NODE_ERROR_BASE_ANALOG);
	SWREG_write_field(&(UNA_DMM_REGISTERS[DMM_REGISTER_ADDRESS_ANALOG_DATA_2]), &unused_mask, UNA_convert_mv(adc_data), DMM_REGISTER_ANALOG_DATA_2_MASK_VUSB);
errors:
	// Turn power block off.
    POWER_disable(POWER_REQUESTER_ID_DMM, POWER_DOMAIN_HMI);
	POWER_disable(POWER_REQUESTER_ID_DMM, POWER_DOMAIN_ANALOG);
	return status;
}

/*******************************************************************/
static UNA_DMM_status_t _UNA_DMM_update_register(uint8_t reg_addr) {
	// Local variables.
	UNA_DMM_status_t status = UNA_DMM_SUCCESS;
	uint32_t unused_mask = 0;
	// Check address.
	switch (reg_addr) {
	case COMMON_REGISTER_ADDRESS_ERROR_STACK:
		// Unstack error.
		SWREG_write_field(&(UNA_DMM_REGISTERS[COMMON_REGISTER_ADDRESS_ERROR_STACK]), &unused_mask, (uint32_t) (ERROR_stack_read()), COMMON_REGISTER_ERROR_STACK_MASK_ERROR);
		break;
	case COMMON_REGISTER_ADDRESS_STATUS_0:
		// Check error stack.
		SWREG_write_field(&(UNA_DMM_REGISTERS[COMMON_REGISTER_ADDRESS_STATUS_0]), &unused_mask, ((ERROR_stack_is_empty() == 0) ? 0b1 : 0b0), COMMON_REGISTER_STATUS_0_MASK_ESF);
		break;
	case DMM_REGISTER_ADDRESS_STATUS_1:
		// Update nodes count.
		SWREG_write_field(&(UNA_DMM_REGISTERS[DMM_REGISTER_ADDRESS_STATUS_1]), &unused_mask, (uint32_t) (NODE_LIST.count), DMM_REGISTER_STATUS_1_MASK_NODES_COUNT);
		break;
	default:
		// Nothing to do on other registers.
		break;
	}
	return status;
}

/*******************************************************************/
static UNA_DMM_status_t _UNA_DMM_check_register(uint8_t reg_addr, uint32_t reg_mask) {
	// Local variables.
	UNA_DMM_status_t status = UNA_DMM_SUCCESS;
	NODE_status_t node_status = NODE_SUCCESS;
	uint32_t reg_value = 0;
	uint32_t unused_mask = 0;
	uint32_t duration = 0;
	// Read register.
	reg_value = UNA_DMM_REGISTERS[reg_addr];
	// Check address.
	switch (reg_addr) {
	case DMM_REGISTER_ADDRESS_CONFIGURATION_0:
		// Check nodes scan period.
		if ((reg_mask & DMM_REGISTER_CONFIGURATION_0_MASK_NODES_SCAN_PERIOD) != 0) {
			// Check range.
			duration = UNA_get_seconds((uint32_t) SWREG_read_field(reg_value, DMM_REGISTER_CONFIGURATION_0_MASK_NODES_SCAN_PERIOD));
			if (duration < UNA_DMM_NODE_SCAN_PERIOD_SECONDS_MIN) {
				// Force minimum value and notify error.
				SWREG_write_field(&(UNA_DMM_REGISTERS[DMM_REGISTER_ADDRESS_CONFIGURATION_0]), &unused_mask, UNA_convert_seconds(UNA_DMM_NODE_SCAN_PERIOD_SECONDS_MIN), DMM_REGISTER_CONFIGURATION_0_MASK_NODES_SCAN_PERIOD);
				status = UNA_DMM_ERROR_SCAN_PERIOD_UNDERFLOW;
				goto errors;
			}
		}
		// Check Sigfox uplink period.
		if ((reg_mask & DMM_REGISTER_CONFIGURATION_0_MASK_UL_PERIOD) != 0) {
			// Check range.
			duration = UNA_get_seconds((uint32_t) SWREG_read_field(reg_value, DMM_REGISTER_CONFIGURATION_0_MASK_UL_PERIOD));
			if (duration < UNA_DMM_RADIO_UL_PERIOD_SECONDS_MIN) {
				// Force minimum value and notify error.
				SWREG_write_field(&(UNA_DMM_REGISTERS[DMM_REGISTER_ADDRESS_CONFIGURATION_0]), &unused_mask, UNA_convert_seconds(UNA_DMM_RADIO_UL_PERIOD_SECONDS_MIN), DMM_REGISTER_CONFIGURATION_0_MASK_UL_PERIOD);
				status = UNA_DMM_ERROR_UPLINK_PERIOD_UNDERFLOW;
				goto errors;
			}
		}
		// Check Sigfox downlink period.
		if ((reg_mask & DMM_REGISTER_CONFIGURATION_0_MASK_DL_PERIOD) != 0) {
			// Check range.
			duration = UNA_get_seconds((uint32_t) SWREG_read_field(reg_value, DMM_REGISTER_CONFIGURATION_0_MASK_DL_PERIOD));
			if (duration < UNA_DMM_RADIO_DL_PERIOD_SECONDS_MIN) {
				// Force minimum value and notify error.
				SWREG_write_field(&(UNA_DMM_REGISTERS[DMM_REGISTER_ADDRESS_CONFIGURATION_0]), &unused_mask, UNA_convert_seconds(UNA_DMM_RADIO_DL_PERIOD_SECONDS_MIN), DMM_REGISTER_CONFIGURATION_0_MASK_DL_PERIOD);
				status = UNA_DMM_ERROR_DOWNLINK_PERIOD_UNDERFLOW;
				goto errors;
			}
		}
		// Store new value in NVM.
		if (reg_mask != 0) {
		    _UNA_DMM_write_nvm_register(reg_addr, reg_value);
		}
		break;
	case COMMON_REGISTER_ADDRESS_CONTROL_0:
		// RTRG.
		if ((reg_mask & COMMON_REGISTER_CONTROL_0_MASK_RTRG) != 0) {
			// Read bit.
			if (SWREG_read_field(reg_value, COMMON_REGISTER_CONTROL_0_MASK_RTRG) != 0) {
				// Reset MCU.
				PWR_software_reset();
			}
		}
		// MTRG.
		if ((reg_mask & COMMON_REGISTER_CONTROL_0_MASK_MTRG) != 0) {
			// Read bit.
			if (SWREG_read_field(reg_value, COMMON_REGISTER_CONTROL_0_MASK_MTRG) != 0) {
				// Clear request.
				SWREG_write_field(&(UNA_DMM_REGISTERS[COMMON_REGISTER_ADDRESS_CONTROL_0]), &unused_mask, 0b0, COMMON_REGISTER_CONTROL_0_MASK_MTRG);
				// Perform measurements.
				status = _UNA_DMM_mtrg_callback();
				if (status != UNA_DMM_SUCCESS) goto errors;
			}
		}
		// BFC.
		if ((reg_mask & COMMON_REGISTER_CONTROL_0_MASK_BFC) != 0) {
			// Read bit.
			if (SWREG_read_field(reg_value, COMMON_REGISTER_CONTROL_0_MASK_BFC) != 0) {
				// Clear request.
				SWREG_write_field(&(UNA_DMM_REGISTERS[COMMON_REGISTER_ADDRESS_CONTROL_0]), &unused_mask, 0b0, COMMON_REGISTER_CONTROL_0_MASK_BFC);
				// Clear boot flag.
				SWREG_write_field(&(UNA_DMM_REGISTERS[COMMON_REGISTER_ADDRESS_STATUS_0]), &unused_mask, 0b0, COMMON_REGISTER_STATUS_0_MASK_BF);
			}
		}
		break;
	case DMM_REGISTER_ADDRESS_CONTROL_1:
		// STRG.
		if ((reg_mask & DMM_REGISTER_CONTROL_1_MASK_STRG) != 0) {
			// Read bit.
			if (SWREG_read_field(reg_value, DMM_REGISTER_CONTROL_1_MASK_STRG) != 0) {
				// Clear request.
				SWREG_write_field(&(UNA_DMM_REGISTERS[DMM_REGISTER_ADDRESS_CONTROL_1]), &unused_mask, 0b0, DMM_REGISTER_CONTROL_1_MASK_STRG);
				// Perform node scanning.
				node_status = NODE_scan();
				NODE_stack_error(ERROR_BASE_NODE);
			}
		}
		break;
	default:
		// Nothing to do for other registers.
		break;
	}
errors:
	return status;
}

/*** UNA DMM functions ***/

/*******************************************************************/
UNA_DMM_status_t UNA_DMM_init(void) {
	// Local variables.
    UNA_DMM_status_t status = UNA_DMM_SUCCESS;
	uint8_t idx = 0;
	uint32_t unused_mask = 0;
#ifdef DMM_NVM_FACTORY_RESET
	// Radio monitoring and node scanning periods.
	SWREG_write_field(&(UNA_DMM_REGISTERS[DMM_REGISTER_ADDRESS_CONFIGURATION_0]), &unused_mask, UNA_convert_seconds(DMM_NODES_SCAN_PERIOD_SECONDS), DMM_REGISTER_CONFIGURATION_0_MASK_NODES_SCAN_PERIOD);
	SWREG_write_field(&(UNA_DMM_REGISTERS[DMM_REGISTER_ADDRESS_CONFIGURATION_0]), &unused_mask, UNA_convert_seconds(DMM_RADIO_UL_PERIOD_SECONDS), DMM_REGISTER_CONFIGURATION_0_MASK_UL_PERIOD);
	SWREG_write_field(&(UNA_DMM_REGISTERS[DMM_REGISTER_ADDRESS_CONFIGURATION_0]), &unused_mask, UNA_convert_seconds(DMM_RADIO_DL_PERIOD_SECONDS), DMM_REGISTER_CONFIGURATION_0_MASK_DL_PERIOD);
	_UNA_DMM_check_register(DMM_REGISTER_ADDRESS_CONFIGURATION_0, UNA_REGISTER_MASK_ALL);
#endif
	// Reset all registers.
	for (idx=0 ; idx<DMM_REGISTER_ADDRESS_LAST ; idx++) {
		UNA_DMM_REGISTERS[idx] = 0;
	}
	// Node ID register.
	SWREG_write_field(&(UNA_DMM_REGISTERS[COMMON_REGISTER_ADDRESS_NODE_ID]), &unused_mask, UNA_NODE_ADDRESS_MASTER, COMMON_REGISTER_NODE_ID_MASK_NODE_ADDR);
	SWREG_write_field(&(UNA_DMM_REGISTERS[COMMON_REGISTER_ADDRESS_NODE_ID]), &unused_mask, UNA_BOARD_ID_DMM, COMMON_REGISTER_NODE_ID_MASK_BOARD_ID);
	// HW version register.
#ifdef HW1_0
	SWREG_write_field(&(UNA_DMM_REGISTERS[COMMON_REGISTER_ADDRESS_HW_VERSION]), &unused_mask, 1, COMMON_REGISTER_HW_VERSION_MASK_MAJOR);
	SWREG_write_field(&(UNA_DMM_REGISTERS[COMMON_REGISTER_ADDRESS_HW_VERSION]), &unused_mask, 0, COMMON_REGISTER_HW_VERSION_MASK_MINOR);
#endif
	// SW version register 0.
	SWREG_write_field(&(UNA_DMM_REGISTERS[COMMON_REGISTER_ADDRESS_SW_VERSION_0]), &unused_mask, GIT_MAJOR_VERSION, COMMON_REGISTER_SW_VERSION_0_MASK_MAJOR);
	SWREG_write_field(&(UNA_DMM_REGISTERS[COMMON_REGISTER_ADDRESS_SW_VERSION_0]), &unused_mask, GIT_MINOR_VERSION, COMMON_REGISTER_SW_VERSION_0_MASK_MINOR);
	SWREG_write_field(&(UNA_DMM_REGISTERS[COMMON_REGISTER_ADDRESS_SW_VERSION_0]), &unused_mask, GIT_COMMIT_INDEX, COMMON_REGISTER_SW_VERSION_0_MASK_COMMIT_INDEX);
	SWREG_write_field(&(UNA_DMM_REGISTERS[COMMON_REGISTER_ADDRESS_SW_VERSION_0]), &unused_mask, GIT_DIRTY_FLAG, COMMON_REGISTER_SW_VERSION_0_MASK_DTYF);
	// SW version register 1.
	SWREG_write_field(&(UNA_DMM_REGISTERS[COMMON_REGISTER_ADDRESS_SW_VERSION_1]), &unused_mask, GIT_COMMIT_ID, COMMON_REGISTER_SW_VERSION_1_MASK_COMMIT_ID);
	// Reset flags registers.
	SWREG_write_field(&(UNA_DMM_REGISTERS[COMMON_REGISTER_ADDRESS_STATUS_0]), &unused_mask, ((uint32_t) (((RCC -> CSR) >> 24) & 0xFF)), COMMON_REGISTER_STATUS_0_MASK_RESET_FLAGS);
	SWREG_write_field(&(UNA_DMM_REGISTERS[COMMON_REGISTER_ADDRESS_STATUS_0]), &unused_mask, 0b1, COMMON_REGISTER_STATUS_0_MASK_BF);
	// Load default values.
	_UNA_DMM_load_dynamic_configuration();
	_UNA_DMM_reset_analog_data();
	return status;
}

/*******************************************************************/
UNA_DMM_status_t UNA_DMM_write_register(UNA_access_parameters_t* write_params, uint32_t reg_value, uint32_t reg_mask, UNA_access_status_t* write_status) {
	// Local variables.
	UNA_DMM_status_t status = UNA_DMM_SUCCESS;
	uint32_t temp = 0;
	// Check parameters.
	if ((write_params == NULL) || (write_status == NULL)) {
		status = UNA_DMM_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Reset access status.
	(write_status -> all) = 0;
	(write_status -> type) = UNA_ACCESS_TYPE_WRITE;
	// Check address.
	if ((write_params -> reg_addr) >= DMM_REGISTER_ADDRESS_LAST) {
		// Act as a slave.
		(*write_status).error_received = 1;
		goto errors;
	}
	if (DMM_REGISTER_ACCESS[(write_params -> reg_addr)] == UNA_REGISTER_ACCESS_READ_ONLY) {
		// Act as a slave.
		(*write_status).error_received = 1;
		goto errors;
	}
	if ((write_params -> node_addr) != UNA_NODE_ADDRESS_MASTER) {
		// Act as a slave.
		(*write_status).reply_timeout = 1;
		goto errors;
	}
	// Read register.
	temp = UNA_DMM_REGISTERS[(write_params -> reg_addr)];
	// Compute new value.
	temp &= ~reg_mask;
	temp |= (reg_value & reg_mask);
	// Write register.
	UNA_DMM_REGISTERS[(write_params -> reg_addr)] = temp;
	// Check control bits.
	status = _UNA_DMM_check_register((write_params -> reg_addr), reg_mask);
	if (status != UNA_DMM_SUCCESS) {
		// Act as a slave.
		(*write_status).error_received = 1;
		goto errors;
	}
errors:
	return status;
}

/*******************************************************************/
UNA_DMM_status_t UNA_DMM_read_register(UNA_access_parameters_t* read_params, uint32_t* reg_value, UNA_access_status_t* read_status) {
	// Local variables.
	UNA_DMM_status_t status = UNA_DMM_SUCCESS;
	// Check parameters.
	if ((read_params == NULL) || (read_status == NULL) || (reg_value == NULL)) {
		status = UNA_DMM_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Reset access status.
	(read_status -> all) = 0;
	(read_status -> type) = UNA_ACCESS_TYPE_READ;
	// Check address.
	if ((read_params -> reg_addr) >= DMM_REGISTER_ADDRESS_LAST) {
		// Act as a slave.
		(*read_status).error_received = 1;
		goto errors;
	}
	if ((read_params -> node_addr) != UNA_NODE_ADDRESS_MASTER) {
		// Act as a slave.
		(*read_status).reply_timeout = 1;
		goto errors;
	}
	// Reset value.
	(*reg_value) = DMM_REGISTER_ERROR_VALUE[(read_params -> reg_addr)];
	// Update register.
	status = _UNA_DMM_update_register(read_params -> reg_addr);
	if (status != UNA_DMM_SUCCESS) {
		// Act as a slave.
		(*read_status).error_received = 1;
		goto errors;
	}
	// Read register.
	(*reg_value) = UNA_DMM_REGISTERS[(read_params -> reg_addr)];
errors:
	return status;
}
