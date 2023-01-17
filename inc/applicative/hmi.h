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
#include "rtc.h"
#include "sh1106.h"

/*** HMI structures ***/

typedef enum {
	HMI_SUCCESS = 0,
	HMI_ERROR_NULL_PARAMETER,
	HMI_ERROR_BASE_I2C = 0x0100,
	HMI_ERROR_BASE_LPTIM = (HMI_ERROR_BASE_I2C + I2C_ERROR_BASE_LAST),
	HMI_ERROR_BASE_RTC = (HMI_ERROR_BASE_LPTIM + LPTIM_ERROR_BASE_LAST),
	HMI_ERROR_BASE_SH1106 = (HMI_ERROR_BASE_RTC + RTC_ERROR_BASE_LAST),
	HMI_ERROR_BASE_LAST = (HMI_ERROR_BASE_SH1106 + SH1106_ERROR_BASE_LAST)
} HMI_status_t;

typedef enum {
	HMI_IRQ_ENCODER_SWITCH = 0,
	HMI_IRQ_ENCODER_FORWARD,
	HMI_IRQ_ENCODER_BACKWARD,
	HMI_IRQ_CMD_ON,
	HMI_IRQ_CMD_OFF,
	HMI_IRQ_BP1,
	HMI_IRQ_BP2,
	HMI_IRQ_BP3,
	HMI_IRQ_LAST
} HMI_irq_flag_t;

/*** HMI functions ***/

void HMI_init(void);
HMI_status_t HMI_task(void);
void HMI_set_irq_flag(HMI_irq_flag_t irq_flag);

#define HMI_status_check(error_base) { if (hmi_status != HMI_SUCCESS) { status = error_base + hmi_status; goto errors; }}
#define HMI_error_check() { ERROR_status_check(hmi_status, HMI_SUCCESS, ERROR_BASE_HMI); }
#define HMI_error_check_print() { ERROR_status_check_print(hmi_status, HMI_SUCCESS, ERROR_BASE_HMI); }

#endif /* __HMI_H__ */
