/*
 * nvic_priority.h
 *
 *  Created on: 15 dec. 2020
 *      Author: Ludo
 */

#ifndef __NVIC_PRIORITY_H__
#define __NVIC_PRIORITY_H__

/*!******************************************************************
 * \enum NVIC_priority_list_t
 * \brief NVIC interrupt priorities list.
 *******************************************************************/
typedef enum {
	// Common.
	NVIC_PRIORITY_CLOCK = 0,
	NVIC_PRIORITY_CLOCK_CALIBRATION = 1,
	NVIC_PRIORITY_DELAY = 2,
	NVIC_PRIORITY_RTC = 3,
	// RS485 interface.
	NVIC_PRIORITY_RS485 = 0,
	// HMI.
	NVIC_PRIORITY_HMI_WAKE_UP = 0,
	NVIC_PRIORITY_HMI_INPUTS = 1,
	NVIC_PRIORITY_HMI_TIMER = 2,
	// LED.
	NVIC_PRIORITY_LED = 1
} NVIC_priority_list_t;

#endif /* __NVIC_PRIORITY_H__ */
