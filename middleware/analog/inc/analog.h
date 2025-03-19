/*
 * analog.h
 *
 *  Created on: 15 dec. 2024
 *      Author: Ludo
 */

#ifndef __ANALOG_H__
#define __ANALOG_H__

#include "adc.h"
#include "error.h"
#include "types.h"

/*** ANALOG structures ***/

/*!******************************************************************
 * \enum ANALOG_status_t
 * \brief ANALOG driver error codes.
 *******************************************************************/
typedef enum {
    // Driver errors.
    ANALOG_SUCCESS = 0,
    ANALOG_ERROR_NULL_PARAMETER,
    ANALOG_ERROR_CHANNEL,
    ANALOG_ERROR_CALIBRATION_MISSING,
    // Low level drivers errors.
    ANALOG_ERROR_BASE_ADC = ERROR_BASE_STEP,
    // Last base value.
    ANALOG_ERROR_BASE_LAST = (ANALOG_ERROR_BASE_ADC + ADC_ERROR_BASE_LAST),
} ANALOG_status_t;

/*!******************************************************************
 * \enum ANALOG_channel_t
 * \brief ANALOG channels list.
 *******************************************************************/
typedef enum {
    ANALOG_CHANNEL_VMCU_MV = 0,
    ANALOG_CHANNEL_TMCU_DEGREES,
    ANALOG_CHANNEL_VRS_MV,
    ANALOG_CHANNEL_VHMI_MV,
    ANALOG_CHANNEL_VUSB_MV,
    ANALOG_CHANNEL_LAST
} ANALOG_channel_t;

/*** ANALOG functions ***/

/*!******************************************************************
 * \fn ANALOG_status_t ANALOG_init(void)
 * \brief Init ANALOG driver.
 * \param[in]   none
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
ANALOG_status_t ANALOG_init(void);

/*!******************************************************************
 * \fn ANALOG_status_t ANALOG_de_init(void)
 * \brief Release ANALOG driver.
 * \param[in]   none
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
ANALOG_status_t ANALOG_de_init(void);

/*!******************************************************************
 * \fn ANALOG_status_t ANALOG_convert_channel(ANALOG_channel_t channel, int32_t* analog_data)
 * \brief Convert an analog channel.
 * \param[in]   channel: Channel to convert.
 * \param[out]  analog_data: Pointer to integer that will contain the result.
 * \retval      Function execution status.
 *******************************************************************/
ANALOG_status_t ANALOG_convert_channel(ANALOG_channel_t channel, int32_t* analog_data);

/*******************************************************************/
#define ANALOG_exit_error(base) { ERROR_check_exit(analog_status, ANALOG_SUCCESS, base) }

/*******************************************************************/
#define ANALOG_stack_error(base) { ERROR_check_stack(analog_status, ANALOG_SUCCESS, base) }

/*******************************************************************/
#define ANALOG_stack_exit_error(base, code) { ERROR_check_stack_exit(analog_status, ANALOG_SUCCESS, base, code) }

#endif /* __ANALOG_H__ */
