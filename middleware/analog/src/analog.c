/*
 * analog.c
 *
 *  Created on: 15 dec. 2024
 *      Author: Ludo
 */

#include "analog.h"

#include "adc.h"
#include "error.h"
#include "mcu_mapping.h"
#include "types.h"

/*** ANALOG local macros ***/

#define ANALOG_VMCU_MV_DEFAULT          3000
#define ANALOG_TMCU_DEGREES_DEFAULT     25

#define ANALOG_VRS_DIVIDER_RATIO        10
#define ANALOG_VHMI_DIVIDER_RATIO       2
#define ANALOG_VUSB_DIVIDER_RATIO       2

#define ANALOG_ERROR_VALUE              0xFFFF

/*** ANALOG local structures ***/

/*******************************************************************/
typedef struct {
    int32_t vmcu_mv;
} ANALOG_context_t;

/*** ANALOG local global variables ***/

static ANALOG_context_t analog_ctx = {
    .vmcu_mv = ANALOG_VMCU_MV_DEFAULT,
};

/*** ANALOG functions ***/

/*******************************************************************/
ANALOG_status_t ANALOG_init(void) {
    // Local variables.
    ANALOG_status_t status = ANALOG_SUCCESS;
    ADC_status_t adc_status = ADC_SUCCESS;
    // Init context.
    analog_ctx.vmcu_mv = ANALOG_VMCU_MV_DEFAULT;
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
    ADC_exit_error(ANALOG_ERROR_BASE_ADC);
errors:
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
    case ANALOG_CHANNEL_VMCU_MV:
        // MCU voltage.
        adc_status = ADC_convert_channel(ADC_CHANNEL_VREFINT, &adc_data_12bits);
        ADC_exit_error(ANALOG_ERROR_BASE_ADC);
        // Convert to mV.
        adc_status = ADC_compute_vmcu(adc_data_12bits, ADC_get_vrefint_voltage_mv(), analog_data);
        ADC_exit_error(ANALOG_ERROR_BASE_ADC);
        // Update local value for temperature computation.
        analog_ctx.vmcu_mv = (*analog_data);
        break;
    case ANALOG_CHANNEL_TMCU_DEGREES:
        // MCU temperature.
        adc_status = ADC_convert_channel(ADC_CHANNEL_TEMPERATURE_SENSOR, &adc_data_12bits);
        ADC_exit_error(ANALOG_ERROR_BASE_ADC);
        // Convert to degrees.
        adc_status = ADC_compute_tmcu(analog_ctx.vmcu_mv, adc_data_12bits, analog_data);
        ADC_exit_error(ANALOG_ERROR_BASE_ADC);
        break;
    case ANALOG_CHANNEL_VRS_MV:
        // Bus voltage.
        adc_status = ADC_convert_channel(ADC_CHANNEL_VRS, &adc_data_12bits);
        ADC_exit_error(ANALOG_ERROR_BASE_ADC);
        // Convert to mV.
        (*analog_data) = (adc_data_12bits * analog_ctx.vmcu_mv * ANALOG_VRS_DIVIDER_RATIO) / (ADC_FULL_SCALE);
        break;
    case ANALOG_CHANNEL_VHMI_MV:
        // Supercap voltage.
        adc_status = ADC_convert_channel(ADC_CHANNEL_VHMI, &adc_data_12bits);
        ADC_exit_error(ANALOG_ERROR_BASE_ADC);
        // Convert to mV.
        (*analog_data) = (adc_data_12bits * analog_ctx.vmcu_mv * ANALOG_VHMI_DIVIDER_RATIO) / (ADC_FULL_SCALE);
        break;
    case ANALOG_CHANNEL_VUSB_MV:
        // Supercap voltage.
        adc_status = ADC_convert_channel(ADC_CHANNEL_VUSB, &adc_data_12bits);
        ADC_exit_error(ANALOG_ERROR_BASE_ADC);
        // Convert to mV.
        (*analog_data) = (adc_data_12bits * analog_ctx.vmcu_mv * ANALOG_VUSB_DIVIDER_RATIO) / (ADC_FULL_SCALE);
        break;
    default:
        status = ANALOG_ERROR_CHANNEL;
        goto errors;
    }
errors:
    return status;
}
