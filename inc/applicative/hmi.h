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

typedef enum {
	HMI_SUCCESS = 0,
	HMI_ERROR_NULL_PARAMETER,
	HMI_ERROR_DATA_DEPTH_OVERFLOW,
	HMI_ERROR_SCREEN,
	HMI_ERROR_BASE_I2C = 0x0100,
	HMI_ERROR_BASE_LPTIM = (HMI_ERROR_BASE_I2C + I2C_ERROR_BASE_LAST),
	HMI_ERROR_BASE_LPUART = (HMI_ERROR_BASE_LPTIM + LPTIM_ERROR_BASE_LAST),
	HMI_ERROR_BASE_TIM = (HMI_ERROR_BASE_LPUART + LPUART_ERROR_BASE_LAST),
	HMI_ERROR_BASE_STRING = (HMI_ERROR_BASE_TIM + TIM_ERROR_BASE_LAST),
	HMI_ERROR_BASE_POWER = (HMI_ERROR_BASE_STRING + STRING_ERROR_BASE_LAST),
	HMI_ERROR_BASE_SH1106 = (HMI_ERROR_BASE_POWER + POWER_ERROR_BASE_LAST),
	HMI_ERROR_BASE_NODE = (HMI_ERROR_BASE_SH1106 + SH1106_ERROR_BASE_LAST),
	HMI_ERROR_BASE_LAST = (HMI_ERROR_BASE_NODE + NODE_ERROR_BASE_LAST)
} HMI_status_t;

/*** HMI functions ***/

void HMI_init_por(void);
void HMI_init(void);
void HMI_de_init(void);
HMI_status_t HMI_task(void);

#define HMI_check_status(error_base) { if (hmi_status != HMI_SUCCESS) { status = error_base + hmi_status; goto errors; }}
#define HMI_stack_error() { ERROR_stack_error(hmi_status, HMI_SUCCESS, ERROR_BASE_HMI); }
#define HMI_print_error() { ERROR_print_error(hmi_status, HMI_SUCCESS, ERROR_BASE_HMI); }

#endif /* __HMI_H__ */
