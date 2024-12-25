/*
 * error.h
 *
 *  Created on: 12 mar. 2022
 *      Author: Ludo
 */

#ifndef __ERROR_BASE_H__
#define __ERROR_BASE_H__

// Peripherals.
#include "adc.h"
#include "flash.h"
#include "i2c.h"
#include "iwdg.h"
#include "lptim.h"
#include "lpuart.h"
#include "nvm.h"
#include "rcc.h"
#include "rtc.h"
#include "tim.h"
// Utils.
#include "math.h"
#include "parser.h"
#include "string.h"
// Components.
#include "led.h"
#include "power.h"
#include "sh1106.h"
// Nodes.
#include "node.h"
#include "una_at.h"
#include "una_dmm.h"
#include "una_r4s8cr.h"
// Middleware.
#include "analog.h"
#include "hmi.h"
#include "radio.h"

/*** ERROR structures ***/

/*!******************************************************************
 * \enum ERROR_base_t
 * \brief Board error bases.
 *******************************************************************/
typedef enum {
	SUCCESS = 0,
	// Peripherals.
	ERROR_BASE_ADC1 = 0x0100,
	ERROR_BASE_FLASH = (ERROR_BASE_ADC1 + ADC_ERROR_BASE_LAST),
	ERROR_BASE_IWDG = (ERROR_BASE_FLASH + FLASH_ERROR_BASE_LAST),
	ERROR_BASE_LPTIM = (ERROR_BASE_IWDG + IWDG_ERROR_BASE_LAST),
	ERROR_BASE_LPUART1 = (ERROR_BASE_LPTIM + LPTIM_ERROR_BASE_LAST),
	ERROR_BASE_NVM = (ERROR_BASE_LPUART1 + LPUART_ERROR_BASE_LAST),
	ERROR_BASE_RCC = (ERROR_BASE_NVM + NVM_ERROR_BASE_LAST),
	ERROR_BASE_RTC = (ERROR_BASE_RCC + RCC_ERROR_BASE_LAST),
	ERROR_BASE_TIM_HMI = (ERROR_BASE_RTC + RTC_ERROR_BASE_LAST),
	ERROR_BASE_TIM_CAL = (ERROR_BASE_TIM_HMI + TIM_ERROR_BASE_LAST),
	ERROR_BASE_TIM_LED_PWM = (ERROR_BASE_TIM_CAL + TIM_ERROR_BASE_LAST),
	ERROR_BASE_TIM_LED_DIMMING = (ERROR_BASE_TIM_LED_PWM + TIM_ERROR_BASE_LAST),
	// Utils.
	ERROR_BASE_MATH = (ERROR_BASE_TIM_LED_DIMMING + TIM_ERROR_BASE_LAST),
	ERROR_BASE_PARSER = (ERROR_BASE_MATH + MATH_ERROR_BASE_LAST),
	ERROR_BASE_STRING = (ERROR_BASE_PARSER + PARSER_ERROR_BASE_LAST),
	// Components.
	ERROR_BASE_LED = (ERROR_BASE_STRING + STRING_ERROR_BASE_LAST),
	ERROR_BASE_POWER = (ERROR_BASE_LED + LED_ERROR_BASE_LAST),
	ERROR_BASE_SH1106 = (ERROR_BASE_POWER + POWER_ERROR_BASE_LAST),
	// Nodes.
	ERROR_BASE_NODE = (ERROR_BASE_SH1106 + SH1106_ERROR_BASE_LAST),
	ERROR_BASE_UNA_AT = (ERROR_BASE_NODE + NODE_ERROR_BASE_LAST),
	ERROR_BASE_UNA_DMM = (ERROR_BASE_UNA_AT + UNA_AT_ERROR_BASE_LAST),
	ERROR_BASE_UNA_R4S8CR = (ERROR_BASE_UNA_DMM + UNA_DMM_ERROR_BASE_LAST),
	// Middleware.
	ERROR_BASE_ANALOG = (ERROR_BASE_UNA_R4S8CR + UNA_R4S8CR_ERROR_BASE_LAST),
	ERROR_BASE_HMI = (ERROR_BASE_ANALOG + ANALOG_ERROR_BASE_LAST),
	ERROR_BASE_RADIO = (ERROR_BASE_HMI + HMI_ERROR_BASE_LAST),
	// Last index.
	ERROR_BASE_LAST = (ERROR_BASE_RADIO + RADIO_ERROR_BASE_LAST)
} ERROR_base_t;

#endif /* __ERROR_BASE_H__ */
