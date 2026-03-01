/*
 * analog.c
 *
 *  Created on: 15 dec. 2024
 *      Author: Ludo
 */

#include "analog.h"

#include "adc.h"
#include "error.h"
#include "error_base.h"
#include "mcu_mapping.h"
#include "types.h"

/*** ANALOG local macros ***/

#define ANALOG_MCU_VOLTAGE_DEFAULT_MV           3000

#define ANALOG_DIVIDER_RATIO_RS485_BUS_VOLTAGE  10
#define ANALOG_DIVIDER_RATIO_HMI_VOLTAGE        2
#define ANALOG_DIVIDER_RATIO_USB_VOLTAGE        2

#define ANALOG_ERROR_VALUE                      0xFFFF

/*** ANALOG local structures ***/

/*******************************************************************/
typedef struct {
    int32_t mcu_voltage_mv;
} ANALOG_context_t;

/*** ANALOG local global variables ***/

static ANALOG_context_t analog_ctx = {
    .mcu_voltage_mv = ANALOG_MCU_VOLTAGE_DEFAULT_MV,
};

/*** ANALOG functions ***/

/*******************************************************************/
ANALOG_status_t ANALOG_init(void) {
    // Local variables.
    ANALOG_status_t status = ANALOG_SUCCESS;
    ADC_status_t adc_status = ADC_SUCCESS;
    // Init context.
    analog_ctx.mcu_voltage_mv = ANALOG_MCU_VOLTAGE_DEFAULT_MV;
    // Init internal ADC.
    adc_status = ADC_init(&ADC_GPIO);
    ADC_exit_error(ANALOG_ERROR_BASE_ADC);
errors:
    return status;
}

/*******************************************************************/
ANALOG_status_t ANALOG_de_init(void) {
    // Local variables.
    ANALOG_status_t status = ANALOG_SUCCESS;
    ADC_status_t adc_status = ADC_SUCCESS;
    // Release internal ADC.
    adc_status = ADC_de_init();
    ADC_stack_error(ERROR_BASE_ANALOG + ANALOG_ERROR_BASE_ADC);
    return status;
}

/*******************************************************************/
ANALOG_status_t ANALOG_convert_channel(ANALOG_channel_t channel, int32_t* analog_data) {
    // Local variables.
    ANALOG_status_t status = ANALOG_SUCCESS;
    ADC_status_t adc_status = ADC_SUCCESS;
    int32_t adc_data_12bits = 0;
    // Check parameter.
    if (analog_data == NULL) {
        status = ANALOG_ERROR_NULL_PARAMETER;
        goto errors;
    }
    // Check channel.
    switch (channel) {
    case ANALOG_CHANNEL_MCU_VOLTAGE_MV:
        // MCU voltage.
        adc_status = ADC_convert_channel(ADC_CHANNEL_VREFINT, &adc_data_12bits);
        ADC_exit_error(ANALOG_ERROR_BASE_ADC);
        // Convert to mV.
        adc_status = ADC_compute_mcu_voltage(adc_data_12bits, ADC_get_vrefint_voltage_mv(), analog_data);
        ADC_exit_error(ANALOG_ERROR_BASE_ADC);
        // Update local value for temperature computation.
        analog_ctx.mcu_voltage_mv = (*analog_data);
        break;
    case ANALOG_CHANNEL_MCU_TEMPERATURE_DEGREES:
        // MCU temperature.
        adc_status = ADC_convert_channel(ADC_CHANNEL_TEMPERATURE_SENSOR, &adc_data_12bits);
        ADC_exit_error(ANALOG_ERROR_BASE_ADC);
        // Convert to degrees.
        adc_status = ADC_compute_mcu_temperature(analog_ctx.mcu_voltage_mv, adc_data_12bits, analog_data);
        ADC_exit_error(ANALOG_ERROR_BASE_ADC);
        break;
    case ANALOG_CHANNEL_RS485_BUS_VOLTAGE_MV:
        // Bus voltage.
        adc_status = ADC_convert_channel(ADC_CHANNEL_RS485_BUS_VOLTAGE, &adc_data_12bits);
        ADC_exit_error(ANALOG_ERROR_BASE_ADC);
        // Convert to mV.
        (*analog_data) = (adc_data_12bits * analog_ctx.mcu_voltage_mv * ANALOG_DIVIDER_RATIO_RS485_BUS_VOLTAGE) / (ADC_FULL_SCALE);
        break;
    case ANALOG_CHANNEL_HMI_VOLTAGE_MV:
        // Supercap voltage.
        adc_status = ADC_convert_channel(ADC_CHANNEL_HMI_VOLTAGE, &adc_data_12bits);
        ADC_exit_error(ANALOG_ERROR_BASE_ADC);
        // Convert to mV.
        (*analog_data) = (adc_data_12bits * analog_ctx.mcu_voltage_mv * ANALOG_DIVIDER_RATIO_HMI_VOLTAGE) / (ADC_FULL_SCALE);
        break;
    case ANALOG_CHANNEL_USB_VOLTAGE_MV:
        // Supercap voltage.
        adc_status = ADC_convert_channel(ADC_CHANNEL_USB_VOLTAGE, &adc_data_12bits);
        ADC_exit_error(ANALOG_ERROR_BASE_ADC);
        // Convert to mV.
        (*analog_data) = (adc_data_12bits * analog_ctx.mcu_voltage_mv * ANALOG_DIVIDER_RATIO_USB_VOLTAGE) / (ADC_FULL_SCALE);
        break;
    default:
        status = ANALOG_ERROR_CHANNEL;
        goto errors;
    }
errors:
    return status;
}
