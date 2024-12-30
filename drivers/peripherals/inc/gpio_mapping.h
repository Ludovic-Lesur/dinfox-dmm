/*
 * gpio_mapping.h
 *
 *  Created on: 18 apr. 2020
 *      Author: Ludo
 */

#ifndef __GPIO_MAPPING_H__
#define __GPIO_MAPPING_H__

#include "adc.h"
#include "gpio.h"
#include "i2c.h"
#include "lpuart.h"
#include "tim.h"

/*** GPIO MAPPING structures ***/

/*******************************************************************/
typedef enum {
    GPIO_TIM_CHANNEL_INDEX_LED_RED = 0,
    GPIO_TIM_CHANNEL_INDEX_LED_GREEN,
    GPIO_TIM_CHANNEL_INDEX_LED_BLUE,
    GPIO_TIM_CHANNEL_INDEX_LAST
} GPIO_tim_channel_t;

/*** GPIO MAPPING global variables ***/

// Analog inputs.
extern const GPIO_pin_t GPIO_MNTR_EN;
extern const ADC_gpio_t GPIO_ADC;
// RS485 interface.
extern const GPIO_pin_t GPIO_RS485_POWER_ENABLE;
extern const LPUART_gpio_t GPIO_RS485_LPUART;
// HMI.
extern const GPIO_pin_t GPIO_HMI_POWER_ENABLE;
extern const I2C_gpio_t GPIO_HMI_I2C;
// Push buttons.
extern const GPIO_pin_t GPIO_BP1;
extern const GPIO_pin_t GPIO_BP2;
extern const GPIO_pin_t GPIO_BP3;
// ON/OFF command switch.
extern const GPIO_pin_t GPIO_CMD_OFF;
extern const GPIO_pin_t GPIO_CMD_ON;
// Rotary encoder.
extern const GPIO_pin_t GPIO_ENC_SW;
extern const GPIO_pin_t GPIO_ENC_CHA;
extern const GPIO_pin_t GPIO_ENC_CHB;
// RGB LED.
extern const TIM_gpio_t GPIO_LED_TIM;
// Test points.
extern const GPIO_pin_t GPIO_TP1;
extern const GPIO_pin_t GPIO_TP2;
extern const GPIO_pin_t GPIO_TP3;

#endif /* __GPIO_MAPPING_H__ */
