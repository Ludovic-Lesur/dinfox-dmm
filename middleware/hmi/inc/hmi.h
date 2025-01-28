/*
 * hmi.h
 *
 *  Created on: 14 Jan. 2023
 *      Author: Ludo
 */

#ifndef __HMI_H__
#define __HMI_H__

#include "i2c.h"
#include "lptim.h"
#include "power.h"
#include "node.h"
#include "rtc.h"
#include "sh1106.h"
#include "string.h"
#include "tim.h"

/*** HMI structures ***/

/*!******************************************************************
 * \enum HMI_status_t
 * \brief HMI driver error codes.
 *******************************************************************/
typedef enum {
    // Driver errors.
    HMI_SUCCESS = 0,
    HMI_ERROR_NULL_PARAMETER,
    HMI_ERROR_NODE_BOARD_ID,
    HMI_ERROR_NODE_NOT_SUPPORTED,
    HMI_ERROR_NODE_LINE_INDEX,
    HMI_ERROR_NODE_ACCESS_TYPE,
    HMI_ERROR_NODE_MEASUREMENTS,
    HMI_ERROR_NODE_WRITE_ACCESS,
    HMI_ERROR_NODE_READ_ACCESS,
    HMI_ERROR_NODE_HMI_NODE_DATA_TYPE,
    HMI_ERROR_DATA_DEPTH_OVERFLOW,
    HMI_ERROR_SCREEN,
    // Low level drivers errors.
    HMI_ERROR_BASE_I2C1 = 0x0100,
    HMI_ERROR_BASE_LPTIM = (HMI_ERROR_BASE_I2C1 + I2C_ERROR_BASE_LAST),
    HMI_ERROR_BASE_TIM = (HMI_ERROR_BASE_LPTIM + LPTIM_ERROR_BASE_LAST),
    HMI_ERROR_BASE_STRING = (HMI_ERROR_BASE_TIM + TIM_ERROR_BASE_LAST),
    HMI_ERROR_BASE_POWER = (HMI_ERROR_BASE_STRING + STRING_ERROR_BASE_LAST),
    HMI_ERROR_BASE_SH1106 = (HMI_ERROR_BASE_POWER + POWER_ERROR_BASE_LAST),
    HMI_ERROR_BASE_NODE = (HMI_ERROR_BASE_SH1106 + SH1106_ERROR_BASE_LAST),
    // Last base value.
    HMI_ERROR_BASE_LAST = (HMI_ERROR_BASE_NODE + NODE_ERROR_BASE_LAST)
} HMI_status_t;

/*** HMI functions ***/

/*!******************************************************************
 * \fn void HMI_init_por(void)
 * \brief POR initialization of the HMI interface.
 * \param[in]   none
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
HMI_status_t HMI_init_por(void);

/*!******************************************************************
 * \fn void HMI_init(void)
 * \brief Init HMI interface.
 * \param[in]   none
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
HMI_status_t HMI_init(void);

/*!******************************************************************
 * \fn void HMI_de_init(void)
 * \brief Release HMI interface.
 * \param[in]   none
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
HMI_status_t HMI_de_init(void);

/*!******************************************************************
 * \fn HMI_status_t HMI_process(void)
 * \brief Main task of HMI interface.
 * \param[in]   none
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
HMI_status_t HMI_process(void);

/*******************************************************************/
#define HMI_exit_error(base) { ERROR_check_exit(hmi_status, HMI_SUCCESS, base) }

/*******************************************************************/
#define HMI_stack_error(base) { ERROR_check_stack(hmi_status, HMI_SUCCESS, base) }

/*******************************************************************/
#define HMI_stack_exit_error(base, code) { ERROR_check_stack_exit(hmi_status, HMI_SUCCESS, base, code) }

#endif /* __HMI_H__ */
