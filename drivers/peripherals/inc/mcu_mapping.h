/*
 * mcu_mapping.h
 *
 *  Created on: 18 apr. 2020
 *      Author: Ludo
 */

#ifndef __MCU_MAPPING_H__
#define __MCU_MAPPING_H__

#include "adc.h"
#include "gpio.h"
#include "i2c.h"
#include "lpuart.h"
#include "tim.h"

/*** MCU MAPPING macros ***/

#define ADC_CHANNEL_VRS             ADC_CHANNEL_IN6
#define ADC_CHANNEL_VHMI            ADC_CHANNEL_IN4
#define ADC_CHANNEL_VUSB            ADC_CHANNEL_IN1

#define I2C_INSTANCE_HMI            I2C_INSTANCE_I2C1

#define TIM_INSTANCE_HMI            TIM_INSTANCE_TIM2
#define TIM_INSTANCE_LED            TIM_INSTANCE_TIM3
#define TIM_INSTANCE_LED_DIMMING    TIM_INSTANCE_TIM22

/*** MCU MAPPING structures ***/

/*******************************************************************/
typedef enum {
    ADC_CHANNEL_INDEX_VRS_MEASURE = 0,
    ADC_CHANNEL_INDEX_VHMI_MEASURE,
    ADC_CHANNEL_INDEX_VUSB_MEASURE,
    ADC_CHANNEL_INDEX_LAST
} ADC_channel_index_t;

/*******************************************************************/
typedef enum {
    TIM_CHANNEL_INDEX_LED_RED = 0,
    TIM_CHANNEL_INDEX_LED_GREEN,
    TIM_CHANNEL_INDEX_LED_BLUE,
    TIM_CHANNEL_INDEX_LED_LAST
} GPIO_tim_channel_led_t;

/*** MCU MAPPING global variables ***/

// Analog inputs.
extern const GPIO_pin_t GPIO_MNTR_EN;
extern const ADC_gpio_t ADC_GPIO;
// RS485 interface.
extern const GPIO_pin_t GPIO_RS485_POWER_ENABLE;
extern const LPUART_gpio_t LPUART_GPIO_RS485;
// HMI.
extern const GPIO_pin_t GPIO_HMI_POWER_ENABLE;
extern const I2C_gpio_t I2C_GPIO_HMI;
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
extern const TIM_gpio_t TIM_GPIO_LED;
// Test points.
extern const GPIO_pin_t GPIO_TP1;
extern const GPIO_pin_t GPIO_TP2;
extern const GPIO_pin_t GPIO_TP3;

#endif /* __MCU_MAPPING_H__ */
