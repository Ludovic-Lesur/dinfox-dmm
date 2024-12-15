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
#include "lpuart.h"
#include "node.h"
#include "power.h"
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
	HMI_ERROR_DATA_DEPTH_OVERFLOW,
	HMI_ERROR_SCREEN,
	// Low level drivers errors.
	HMI_ERROR_BASE_I2C1 = 0x0100,
	HMI_ERROR_BASE_LPTIM1 = (HMI_ERROR_BASE_I2C1 + I2C_ERROR_BASE_LAST),
	HMI_ERROR_BASE_LPUART1 = (HMI_ERROR_BASE_LPTIM1 + LPTIM_ERROR_BASE_LAST),
	HMI_ERROR_BASE_TIM2 = (HMI_ERROR_BASE_LPUART1 + LPUART_ERROR_BASE_LAST),
	HMI_ERROR_BASE_STRING = (HMI_ERROR_BASE_TIM2 + TIM_ERROR_BASE_LAST),
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
 * \param[in]  	none
 * \param[out] 	none
 * \retval		none
 *******************************************************************/
void HMI_init_por(void);

/*!******************************************************************
 * \fn void HMI_init(void)
 * \brief Init HMI interface.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		none
 *******************************************************************/
void HMI_init(void);

/*!******************************************************************
 * \fn void HMI_de_init(void)
 * \brief Release HMI interface.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		none
 *******************************************************************/
void HMI_de_init(void);

/*!******************************************************************
 * \fn HMI_status_t HMI_process(void)
 * \brief Main task of HMI interface.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		Function execution status.
 *******************************************************************/
HMI_status_t HMI_process(void);

/*******************************************************************/
#define HMI_exit_error(error_base) { if (hmi_status != HMI_SUCCESS) { status = (error_base + hmi_status); goto errors; } }

/*******************************************************************/
#define HMI_stack_error(void) { if (hmi_status != HMI_SUCCESS) { ERROR_stack_add(ERROR_BASE_HMI + hmi_status); } }

/*******************************************************************/
#define HMI_stack_exit_error(error_code) { if (hmi_status != HMI_SUCCESS) { ERROR_stack_add(ERROR_BASE_HMI + hmi_status); status = error_code; goto errors; } }

#endif /* __HMI_H__ */
