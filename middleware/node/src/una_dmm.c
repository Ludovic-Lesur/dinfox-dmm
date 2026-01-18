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
#include "swreg.h"
#include "version.h"
#include "una.h"

/*** UNA DMM local macros ***/

#define UNA_DMM_NODE_SCAN_PERIOD_SECONDS_MIN        3600
#define UNA_DMM_NODE_SCAN_PERIOD_SECONDS_MAX        2592000
#define UNA_DMM_NODE_SCAN_PERIOD_SECONDS_DEFAULT    86400

#define UNA_DMM_SIGFOX_UL_PERIOD_SECONDS_MIN        60
#define UNA_DMM_SIGFOX_UL_PERIOD_SECONDS_MAX        86400
#define UNA_DMM_SIGFOX_UL_PERIOD_SECONDS_DEFAULT    300

#define UNA_DMM_SIGFOX_DL_PERIOD_SECONDS_MIN        600
#define UNA_DMM_SIGFOX_DL_PERIOD_SECONDS_MAX        604800
#define UNA_DMM_SIGFOX_DL_PERIOD_SECONDS_DEFAULT    21600

#ifdef HW1_0
#define UNA_DMM_HW_VERSION_MAJOR                    1
#define UNA_DMM_HW_VERSION_MINOR                    0
#endif
#ifdef DMM_DEBUG
#define UNA_DMM_FLAG_DF                             0b1
#else
#define UNA_DMM_FLAG_DF                             0b0
#endif
#ifdef DMM_NVM_FACTORY_RESET
#define UNA_DMM_FLAG_NFRF                           0b1
#else
#define UNA_DMM_FLAG_NFRF                           0b0
#endif

/*** UNA DMM local structures ***/

/*******************************************************************/
typedef struct {
    uint8_t internal_access;
} UNA_DMM_context_t;

/*** UNA DMM local global variables ***/

static uint32_t UNA_DMM_RAM_REGISTER[DMM_REGISTER_ADDRESS_LAST] = { [0 ... (DMM_REGISTER_ADDRESS_LAST - 1)] = 0x00000000 };

static UNA_DMM_context_t una_dmm_ctx = {
    .internal_access = 0
};

/*** UNA DMM local functions ***/

#ifndef DMM_NVM_FACTORY_RESET
/*******************************************************************/
static UNA_DMM_status_t _UNA_DMM_load_register(uint8_t reg_addr, uint32_t* reg_value) {
    // Local variables.
    UNA_DMM_status_t status = NODE_SUCCESS;
    NVM_status_t nvm_status = NVM_SUCCESS;
    uint8_t nvm_byte = 0;
    uint8_t idx = 0;
    // Byte loop.
    for (idx = 0; idx < UNA_REGISTER_SIZE_BYTES; idx++) {
        // Read NVM.
        nvm_status = NVM_read_byte((NVM_ADDRESS_UNA_REGISTERS + (reg_addr << 2) + idx), &nvm_byte);
        NVM_exit_error(UNA_DMM_ERROR_BASE_NVM);
        // Update output value.
        (*reg_value) |= ((uint32_t) nvm_byte) << (idx << 3);
    }
errors:
    return status;
}
#endif

/*******************************************************************/
static UNA_DMM_status_t _UNA_DMM_store_register(uint8_t reg_addr) {
    // Local variables.
    UNA_DMM_status_t status = UNA_DMM_SUCCESS;
    NVM_status_t nvm_status = NVM_SUCCESS;
    uint8_t nvm_byte = 0;
    uint8_t idx = 0;
    // Byte loop.
    for (idx = 0; idx < 4; idx++) {
        // Compute byte.
        nvm_byte = (uint8_t) (((UNA_DMM_RAM_REGISTER[reg_addr]) >> (idx << 3)) & 0x000000FF);
        // Write NVM.
        nvm_status = NVM_write_byte((NVM_ADDRESS_UNA_REGISTERS + (reg_addr << 2) + idx), nvm_byte);
        NVM_exit_error(UNA_DMM_ERROR_BASE_NVM);
    }
errors:
    return status;
}

/*******************************************************************/
static void _UNA_DMM_init_register(uint8_t reg_addr, uint32_t* reg_value) {
    // Local variables.
    uint32_t unused_mask = 0;
    // Check address.
    switch (reg_addr) {
    case COMMON_REGISTER_ADDRESS_NODE_ID:
        SWREG_write_field(reg_value, &unused_mask, (uint32_t) UNA_NODE_ADDRESS_MASTER, COMMON_REGISTER_NODE_ID_MASK_NODE_ADDR);
        SWREG_write_field(reg_value, &unused_mask, (uint32_t) UNA_BOARD_ID_DMM, COMMON_REGISTER_NODE_ID_MASK_BOARD_ID);
        break;
    case COMMON_REGISTER_ADDRESS_HW_VERSION:
        SWREG_write_field(reg_value, &unused_mask, UNA_DMM_HW_VERSION_MAJOR, COMMON_REGISTER_HW_VERSION_MASK_MAJOR);
        SWREG_write_field(reg_value, &unused_mask, UNA_DMM_HW_VERSION_MINOR, COMMON_REGISTER_HW_VERSION_MASK_MINOR);
        break;
    case COMMON_REGISTER_ADDRESS_SW_VERSION_0:
        SWREG_write_field(reg_value, &unused_mask, (uint32_t) GIT_MAJOR_VERSION, COMMON_REGISTER_SW_VERSION_0_MASK_MAJOR);
        SWREG_write_field(reg_value, &unused_mask, (uint32_t) GIT_MINOR_VERSION, COMMON_REGISTER_SW_VERSION_0_MASK_MINOR);
        SWREG_write_field(reg_value, &unused_mask, (uint32_t) GIT_COMMIT_INDEX, COMMON_REGISTER_SW_VERSION_0_MASK_COMMIT_INDEX);
        SWREG_write_field(reg_value, &unused_mask, (uint32_t) GIT_DIRTY_FLAG, COMMON_REGISTER_SW_VERSION_0_MASK_DTYF);
        break;
    case COMMON_REGISTER_ADDRESS_SW_VERSION_1:
        SWREG_write_field(reg_value, &unused_mask, (uint32_t) GIT_COMMIT_ID, COMMON_REGISTER_SW_VERSION_1_MASK_COMMIT_ID);
        break;
    case COMMON_REGISTER_ADDRESS_FLAGS_0:
        SWREG_write_field(reg_value, &unused_mask, UNA_DMM_FLAG_DF, COMMON_REGISTER_FLAGS_0_MASK_DF);
        SWREG_write_field(reg_value, &unused_mask, UNA_DMM_FLAG_NFRF, COMMON_REGISTER_FLAGS_0_MASK_NFRF);
        break;
    case COMMON_REGISTER_ADDRESS_STATUS_0:
        SWREG_write_field(reg_value, &unused_mask, (uint32_t) PWR_get_reset_flags(), COMMON_REGISTER_STATUS_0_MASK_RESET_FLAGS);
        SWREG_write_field(reg_value, &unused_mask, 0b1, COMMON_REGISTER_STATUS_0_MASK_BF);
        break;
#ifdef DMM_NVM_FACTORY_RESET
    case DMM_REGISTER_ADDRESS_CONFIGURATION_0:
        SWREG_write_field(reg_value, &unused_mask, UNA_convert_seconds(DMM_NODES_SCAN_PERIOD_SECONDS), DMM_REGISTER_CONFIGURATION_0_MASK_NODES_SCAN_PERIOD);
        SWREG_write_field(reg_value, &unused_mask, UNA_convert_seconds(DMM_SIGFOX_UL_PERIOD_SECONDS), DMM_REGISTER_CONFIGURATION_0_MASK_SIGFOX_UL_PERIOD);
        SWREG_write_field(reg_value, &unused_mask, UNA_convert_seconds(DMM_SIGFOX_DL_PERIOD_SECONDS), DMM_REGISTER_CONFIGURATION_0_MASK_SIGFOX_DL_PERIOD);
        break;
#endif
    default:
        break;
    }
}

/*******************************************************************/
static void _UNA_DMM_refresh_register(uint8_t reg_addr) {
    // Local variables.
    uint32_t* reg_ptr = &(UNA_DMM_RAM_REGISTER[reg_addr]);
    uint32_t unused_mask = 0;
    // Check address.
    switch (reg_addr) {
    case COMMON_REGISTER_ADDRESS_ERROR_STACK:
        SWREG_write_field(reg_ptr, &unused_mask, (uint32_t) (ERROR_stack_read()), COMMON_REGISTER_ERROR_STACK_MASK_ERROR);
        break;
    case COMMON_REGISTER_ADDRESS_STATUS_0:
        SWREG_write_field(reg_ptr, &unused_mask, ((ERROR_stack_is_empty() == 0) ? 0b1 : 0b0), COMMON_REGISTER_STATUS_0_MASK_ESF);
        break;
    case DMM_REGISTER_ADDRESS_STATUS_1:
        SWREG_write_field(reg_ptr, &unused_mask, (uint32_t) (NODE_LIST.count), DMM_REGISTER_STATUS_1_MASK_NODES_COUNT);
        break;
    default:
        break;
    }
}

/*******************************************************************/
static UNA_DMM_status_t _UNA_DMM_secure_register(uint8_t reg_addr, uint32_t new_reg_value, uint32_t* reg_mask, uint32_t* reg_value) {
    // Local variables.
    UNA_DMM_status_t status = UNA_DMM_SUCCESS;
    uint32_t generic_u32 = 0;
    // Check address.
    switch (reg_addr) {
    case COMMON_REGISTER_ADDRESS_NODE_ID:
        SWREG_secure_field(
            COMMON_REGISTER_NODE_ID_MASK_NODE_ADDR,,,
            <= UNA_NODE_ADDRESS_RS485_BRIDGE,
            >= UNA_NODE_ADDRESS_R4S8CR_START,
            UNA_NODE_ADDRESS_ERROR,
            status = UNA_DMM_ERROR_REGISTER_FIELD_VALUE
        );
        SWREG_secure_field(
            COMMON_REGISTER_NODE_ID_MASK_BOARD_ID,,,
            >= UNA_BOARD_ID_LAST,
            >= UNA_BOARD_ID_LAST,
            UNA_BOARD_ID_ERROR,
            status = UNA_DMM_ERROR_REGISTER_FIELD_VALUE
        );
        break;
    case DMM_REGISTER_ADDRESS_CONFIGURATION_0:
        SWREG_secure_field(
            DMM_REGISTER_CONFIGURATION_0_MASK_NODES_SCAN_PERIOD,
            UNA_get_seconds,
            UNA_convert_seconds,
            < UNA_DMM_NODE_SCAN_PERIOD_SECONDS_MIN,
            > UNA_DMM_NODE_SCAN_PERIOD_SECONDS_MAX,
            UNA_DMM_NODE_SCAN_PERIOD_SECONDS_DEFAULT,
            status = UNA_DMM_ERROR_REGISTER_FIELD_VALUE
        );
        SWREG_secure_field(
            DMM_REGISTER_CONFIGURATION_0_MASK_SIGFOX_UL_PERIOD,
            UNA_get_seconds,
            UNA_convert_seconds,
            < UNA_DMM_SIGFOX_UL_PERIOD_SECONDS_MIN,
            > UNA_DMM_SIGFOX_UL_PERIOD_SECONDS_MAX,
            UNA_DMM_SIGFOX_UL_PERIOD_SECONDS_DEFAULT,
            status = UNA_DMM_ERROR_REGISTER_FIELD_VALUE
        );
        SWREG_secure_field(
            DMM_REGISTER_CONFIGURATION_0_MASK_SIGFOX_DL_PERIOD,
            UNA_get_seconds,
            UNA_convert_seconds,
            < UNA_DMM_SIGFOX_DL_PERIOD_SECONDS_MIN,
            > UNA_DMM_SIGFOX_DL_PERIOD_SECONDS_MAX,
            UNA_DMM_SIGFOX_DL_PERIOD_SECONDS_DEFAULT,
            status = UNA_DMM_ERROR_REGISTER_FIELD_VALUE
        );
        break;
    default:
        break;
    }
    return status;
}

/*******************************************************************/
static UNA_DMM_status_t _UNA_DMM_mtrg_callback(void) {
    // Local variables.
    UNA_DMM_status_t status = UNA_DMM_SUCCESS;
    ANALOG_status_t analog_status = ANALOG_SUCCESS;
    uint32_t* reg_analog_data_0_ptr = &(UNA_DMM_RAM_REGISTER[COMMON_REGISTER_ADDRESS_ANALOG_DATA_0]);
    uint32_t* reg_analog_data_1_ptr = &(UNA_DMM_RAM_REGISTER[DMM_REGISTER_ADDRESS_ANALOG_DATA_1]);
    uint32_t* reg_analog_data_2_ptr = &(UNA_DMM_RAM_REGISTER[DMM_REGISTER_ADDRESS_ANALOG_DATA_2]);
    int32_t adc_data = 0;
    uint32_t unused_mask = 0;
    // Reset data.
    (*reg_analog_data_0_ptr) = DMM_REGISTER[COMMON_REGISTER_ADDRESS_ANALOG_DATA_0].error_value;
    (*reg_analog_data_1_ptr) = DMM_REGISTER[DMM_REGISTER_ADDRESS_ANALOG_DATA_1].error_value;
    (*reg_analog_data_2_ptr) = DMM_REGISTER[DMM_REGISTER_ADDRESS_ANALOG_DATA_2].error_value;
    // Turn ADC and HMI on.
    POWER_enable(POWER_REQUESTER_ID_DMM, POWER_DOMAIN_HMI, LPTIM_DELAY_MODE_STOP);
    POWER_enable(POWER_REQUESTER_ID_DMM, POWER_DOMAIN_ANALOG, LPTIM_DELAY_MODE_ACTIVE);
    // VMCU.
    analog_status = ANALOG_convert_channel(ANALOG_CHANNEL_VMCU_MV, &adc_data);
    ANALOG_exit_error(NODE_ERROR_BASE_ANALOG);
    SWREG_write_field(reg_analog_data_0_ptr, &unused_mask, UNA_convert_mv(adc_data), COMMON_REGISTER_ANALOG_DATA_0_MASK_VMCU);
    // TMCU.
    analog_status = ANALOG_convert_channel(ANALOG_CHANNEL_TMCU_DEGREES, &adc_data);
    ANALOG_exit_error(NODE_ERROR_BASE_ANALOG);
    SWREG_write_field(reg_analog_data_0_ptr, &unused_mask, UNA_convert_tenth_degrees(adc_data * 10), COMMON_REGISTER_ANALOG_DATA_0_MASK_TMCU);
    // VRS.
    analog_status = ANALOG_convert_channel(ANALOG_CHANNEL_VRS_MV, &adc_data);
    ANALOG_exit_error(NODE_ERROR_BASE_ANALOG);
    SWREG_write_field(reg_analog_data_1_ptr, &unused_mask, UNA_convert_mv(adc_data), DMM_REGISTER_ANALOG_DATA_1_MASK_VRS);
    // VHMI.
    analog_status = ANALOG_convert_channel(ANALOG_CHANNEL_VHMI_MV, &adc_data);
    ANALOG_exit_error(NODE_ERROR_BASE_ANALOG);
    SWREG_write_field(reg_analog_data_1_ptr, &unused_mask, UNA_convert_mv(adc_data), DMM_REGISTER_ANALOG_DATA_1_MASK_VHMI);
    // VUSB.
    analog_status = ANALOG_convert_channel(ANALOG_CHANNEL_VUSB_MV, &adc_data);
    ANALOG_exit_error(NODE_ERROR_BASE_ANALOG);
    SWREG_write_field(reg_analog_data_2_ptr, &unused_mask, UNA_convert_mv(adc_data), DMM_REGISTER_ANALOG_DATA_2_MASK_VUSB);
errors:
    // Turn power block off.
    POWER_disable(POWER_REQUESTER_ID_DMM, POWER_DOMAIN_HMI);
    POWER_disable(POWER_REQUESTER_ID_DMM, POWER_DOMAIN_ANALOG);
    return status;
}

/*******************************************************************/
static UNA_DMM_status_t _UNA_DMM_process_register(uint8_t reg_addr, uint32_t reg_mask) {
    // Local variables.
    UNA_DMM_status_t status = UNA_DMM_SUCCESS;
    NODE_status_t node_status = NODE_SUCCESS;
    uint32_t* reg_ptr = &(UNA_DMM_RAM_REGISTER[reg_addr]);
    uint32_t unused_mask = 0;
    // Check address.
    switch (reg_addr) {
    case COMMON_REGISTER_ADDRESS_CONTROL_0:
        // RTRG.
        if ((reg_mask & COMMON_REGISTER_CONTROL_0_MASK_RTRG) != 0) {
            // Read bit.
            if (SWREG_read_field((*reg_ptr), COMMON_REGISTER_CONTROL_0_MASK_RTRG) != 0) {
                // Reset MCU.
                PWR_software_reset();
            }
        }
        // MTRG.
        if ((reg_mask & COMMON_REGISTER_CONTROL_0_MASK_MTRG) != 0) {
            // Read bit.
            if (SWREG_read_field((*reg_ptr), COMMON_REGISTER_CONTROL_0_MASK_MTRG) != 0) {
                // Clear request.
                SWREG_write_field(reg_ptr, &unused_mask, 0b0, COMMON_REGISTER_CONTROL_0_MASK_MTRG);
                // Perform measurements.
                status = _UNA_DMM_mtrg_callback();
                if (status != UNA_DMM_SUCCESS) goto errors;
            }
        }
        // BFC.
        if ((reg_mask & COMMON_REGISTER_CONTROL_0_MASK_BFC) != 0) {
            // Read bit.
            if (SWREG_read_field((*reg_ptr), COMMON_REGISTER_CONTROL_0_MASK_BFC) != 0) {
                // Clear request and boot flag.
                SWREG_write_field(reg_ptr, &unused_mask, 0b0, COMMON_REGISTER_CONTROL_0_MASK_BFC);
                SWREG_write_field(&(UNA_DMM_RAM_REGISTER[COMMON_REGISTER_ADDRESS_STATUS_0]), &unused_mask, 0b0, COMMON_REGISTER_STATUS_0_MASK_BF);
                // Clear MCU reset flags.
                PWR_clear_reset_flags();
            }
        }
        break;
    case DMM_REGISTER_ADDRESS_CONTROL_1:
        // STRG.
        if ((reg_mask & DMM_REGISTER_CONTROL_1_MASK_STRG) != 0) {
            // Read bit.
            if (SWREG_read_field((*reg_ptr), DMM_REGISTER_CONTROL_1_MASK_STRG) != 0) {
                // Clear request.
                SWREG_write_field(reg_ptr, &unused_mask, 0b0, DMM_REGISTER_CONTROL_1_MASK_STRG);
                // Perform node scanning.
                node_status = NODE_scan();
                NODE_stack_error(ERROR_BASE_NODE);
            }
        }
        break;
    default:
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
    UNA_DMM_status_t una_dmm_status = UNA_DMM_SUCCESS;
    uint32_t init_reg_value = 0;
    uint8_t reg_addr = 0;
    UNA_access_parameters_t write_params;
    UNA_access_status_t unused_status;
    // Init context.
    una_dmm_ctx.internal_access = 1;
    // Init registers.
    for (reg_addr = 0; reg_addr < DMM_REGISTER_ADDRESS_LAST; reg_addr++) {
        // Check reset value.
        switch (DMM_REGISTER[reg_addr].reset_value) {
        case UNA_REGISTER_RESET_VALUE_STATIC:
            // Init to error value.
            init_reg_value = DMM_REGISTER[reg_addr].error_value;
            break;
#ifndef DMM_NVM_FACTORY_RESET
        case UNA_REGISTER_RESET_VALUE_NVM:
            // Read NVM.
            una_dmm_status = _UNA_DMM_load_register(reg_addr, &init_reg_value);
            UNA_DMM_stack_error(ERROR_BASE_UNA_DMM);
            break;
#endif
        default:
            // Init to default value.
            init_reg_value = 0x00000000;
            break;
        }
        // Override with specific initialization value.
        _UNA_DMM_init_register(reg_addr, &init_reg_value);
        // Init write parameters.
        write_params.node_addr = UNA_NODE_ADDRESS_MASTER;
        write_params.reg_addr = reg_addr;
        write_params.reply_params.type = UNA_REPLY_TYPE_NONE;
        write_params.reply_params.timeout_ms = DMM_REGISTER[reg_addr].timeout_ms;
        // Write initial value.
        una_dmm_status = UNA_DMM_write_register(&write_params, init_reg_value, UNA_REGISTER_MASK_ALL, &unused_status);
        UNA_DMM_stack_error(ERROR_BASE_UNA_DMM);
    }
    una_dmm_ctx.internal_access = 0;
    return status;
}

/*******************************************************************/
UNA_DMM_status_t UNA_DMM_write_register(UNA_access_parameters_t* write_params, uint32_t reg_value, uint32_t reg_mask, UNA_access_status_t* write_status) {
    // Local variables.
    UNA_DMM_status_t status = UNA_DMM_SUCCESS;
    UNA_DMM_status_t una_dmm_status = UNA_DMM_SUCCESS;
    uint8_t reg_addr = (write_params->reg_addr);
    uint32_t safe_reg_mask = reg_mask;
    uint32_t safe_reg_value = reg_value;
    // Check parameters.
    if ((write_params == NULL) || (write_status == NULL)) {
        status = UNA_DMM_ERROR_NULL_PARAMETER;
        goto errors;
    }
    // Reset access status.
    (write_status->all) = 0;
    (write_status->type) = UNA_ACCESS_TYPE_WRITE;
    // Check node address.
    if ((write_params->node_addr) != UNA_NODE_ADDRESS_MASTER) {
       // Act as a slave.
       (*write_status).reply_timeout = 1;
       goto errors;
    }
    // Check register address.
    if (reg_addr >= DMM_REGISTER_ADDRESS_LAST) {
        // Act as a slave.
        (*write_status).error_received = 1;
        goto errors;
    }
    // Check access.
    if ((una_dmm_ctx.internal_access == 0) && (DMM_REGISTER[reg_addr].access == UNA_REGISTER_ACCESS_READ_ONLY)) {
        // Act as a slave.
        (*write_status).error_received = 1;
        goto errors;
    }
    // Check input value.
    una_dmm_status = _UNA_DMM_secure_register(reg_addr, reg_value, &safe_reg_mask, &safe_reg_value);
    UNA_DMM_stack_error(ERROR_BASE_UNA_DMM);
    // Write RAM register.
    SWREG_modify_register((uint32_t*) &(UNA_DMM_RAM_REGISTER[reg_addr]), reg_value, safe_reg_mask);
    // Secure register.
    una_dmm_status = _UNA_DMM_secure_register(reg_addr, UNA_DMM_RAM_REGISTER[reg_addr], &safe_reg_mask, &(UNA_DMM_RAM_REGISTER[reg_addr]));
    UNA_DMM_stack_error(ERROR_BASE_UNA_DMM);
    // Store value in NVM if needed.
    if (DMM_REGISTER[reg_addr].reset_value == UNA_REGISTER_RESET_VALUE_NVM) {
        una_dmm_status = _UNA_DMM_store_register(reg_addr);
        UNA_DMM_stack_error(ERROR_BASE_UNA_DMM);
    }
    // Check actions.
    if (una_dmm_ctx.internal_access == 0) {
        una_dmm_status = _UNA_DMM_process_register(reg_addr, reg_mask);
        UNA_DMM_stack_error(ERROR_BASE_UNA_DMM);
        // Act as a slave.
        if (una_dmm_status != UNA_DMM_SUCCESS) {
            (*write_status).error_received = 1;
        }
    }
errors:
    return status;
}

/*******************************************************************/
UNA_DMM_status_t UNA_DMM_read_register(UNA_access_parameters_t* read_params, uint32_t* reg_value, UNA_access_status_t* read_status) {
    // Local variables.
    UNA_DMM_status_t status = UNA_DMM_SUCCESS;
    uint8_t reg_addr = (read_params->reg_addr);
    // Check parameters.
    if ((read_params == NULL) || (read_status == NULL) || (reg_value == NULL)) {
        status = UNA_DMM_ERROR_NULL_PARAMETER;
        goto errors;
    }
    // Reset access status.
    (read_status->all) = 0;
    (read_status->type) = UNA_ACCESS_TYPE_READ;
    // Check node address.
    if ((read_params->node_addr) != UNA_NODE_ADDRESS_MASTER) {
        // Act as a slave.
        (*read_status).reply_timeout = 1;
        goto errors;
    }
    // Check register address.
    if (reg_addr >= DMM_REGISTER_ADDRESS_LAST) {
        // Act as a slave.
        (*read_status).error_received = 1;
        goto errors;
    }
    _UNA_DMM_refresh_register(reg_addr);
    // Read register.
    (*reg_value) = UNA_DMM_RAM_REGISTER[reg_addr];
errors:
    return status;
}
