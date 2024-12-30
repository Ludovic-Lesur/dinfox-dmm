/*
 * gpio_mapping.c
 *
 *  Created on: 17 apr. 2024
 *      Author: Ludo
 */

#include "gpio_mapping.h"

#include "adc.h"
#include "gpio.h"
#include "gpio_reg.h"
#include "i2c.h"
#include "lpuart.h"
#include "tim.h"

/*** GPIO MAPPING local structures ***/

/*******************************************************************/
typedef enum {
    GPIO_ADC_CHANNEL_INDEX_VRS_MEASURE = 0,
    GPIO_ADC_CHANNEL_INDEX_VHMI_MEASURE,
    GPIO_ADC_CHANNEL_INDEX_VUSB_MEASURE,
    GPIO_ADC_CHANNEL_INDEX_LAST
} GPIO_adc_channel_index_t;

/*** GPIO MAPPING local global variables ***/

// Analog inputs.
static const GPIO_pin_t GPIO_ADC_VRS_MEASURE = (GPIO_pin_t) { GPIOA, 0, 6, 0 };
static const GPIO_pin_t GPIO_ADC_VHMI_MEASURE = (GPIO_pin_t) { GPIOA, 0, 4, 0 };
static const GPIO_pin_t GPIO_ADC_VUSB_MEASURE = (GPIO_pin_t) { GPIOA, 0, 1, 0 };
// Analog inputs list.
static const GPIO_pin_t* GPIO_ADC_PINS_LIST[GPIO_ADC_CHANNEL_INDEX_LAST] = { &GPIO_ADC_VRS_MEASURE, &GPIO_ADC_VHMI_MEASURE, &GPIO_ADC_VUSB_MEASURE };
// LPUART1.
static const GPIO_pin_t GPIO_LPUART1_TX = (GPIO_pin_t) { GPIOB, 1, 10, 4 };
static const GPIO_pin_t GPIO_LPUART1_RX = (GPIO_pin_t) { GPIOB, 1, 11, 4 };
static const GPIO_pin_t GPIO_LPUART1_DE = (GPIO_pin_t) { GPIOB, 1, 12, 2 };
static const GPIO_pin_t GPIO_LPUART1_NRE = (GPIO_pin_t) { GPIOB, 1, 2, 0 };
// I2C1.
static const GPIO_pin_t GPIO_I2C1_SCL = (GPIO_pin_t) { GPIOB, 1, 6, 1 };
static const GPIO_pin_t GPIO_I2C1_SDA = (GPIO_pin_t) { GPIOB, 1, 7, 1 };
// RGB LED.
static const GPIO_pin_t GPIO_LED_RED = (GPIO_pin_t) { GPIOA, 0, 7, 2 };
static const GPIO_pin_t GPIO_LED_GREEN = (GPIO_pin_t) { GPIOB, 1, 0, 2 };
static const GPIO_pin_t GPIO_LED_BLUE = (GPIO_pin_t) { GPIOB, 1, 1, 2 };
// Timer channels.
static const TIM_channel_gpio_t GPIO_TIM_CHANNEL_RED = { TIM_CHANNEL_2, &GPIO_LED_RED, TIM_POLARITY_ACTIVE_LOW };
static const TIM_channel_gpio_t GPIO_TIM_CHANNEL_GREEN = { TIM_CHANNEL_3, &GPIO_LED_GREEN, TIM_POLARITY_ACTIVE_LOW };
static const TIM_channel_gpio_t GPIO_TIM_CHANNEL_BLUE = { TIM_CHANNEL_4, &GPIO_LED_BLUE, TIM_POLARITY_ACTIVE_LOW };
// Timer pins list.
static const TIM_channel_gpio_t* GPIO_TIM_PINS_LIST[GPIO_TIM_CHANNEL_INDEX_LAST] = { &GPIO_TIM_CHANNEL_RED, &GPIO_TIM_CHANNEL_GREEN, &GPIO_TIM_CHANNEL_BLUE };

/*** GPIO MAPPING global variables ***/

// Analog inputs.
const GPIO_pin_t GPIO_MNTR_EN = (GPIO_pin_t) { GPIOA, 0, 5, 0 };
const ADC_gpio_t GPIO_ADC = { (const GPIO_pin_t**) &GPIO_ADC_PINS_LIST, GPIO_ADC_CHANNEL_INDEX_LAST };
// RS485 interface.
const GPIO_pin_t GPIO_RS485_POWER_ENABLE = (GPIO_pin_t) { GPIOA, 0, 8, 0 };
const LPUART_gpio_t GPIO_RS485_LPUART = { &GPIO_LPUART1_TX, &GPIO_LPUART1_RX, &GPIO_LPUART1_DE, &GPIO_LPUART1_NRE };
// HMI.
const GPIO_pin_t GPIO_HMI_POWER_ENABLE = (GPIO_pin_t) { GPIOC, 2, 13, 0 };
const I2C_gpio_t GPIO_HMI_I2C = { &GPIO_I2C1_SCL, &GPIO_I2C1_SDA };
// Push buttons.
const GPIO_pin_t GPIO_BP1 = (GPIO_pin_t) { GPIOB, 1, 8, 0 };
const GPIO_pin_t GPIO_BP2 = (GPIO_pin_t) { GPIOB, 1, 15, 0 };
const GPIO_pin_t GPIO_BP3 = (GPIO_pin_t) { GPIOB, 1, 9, 0 };
// ON/OFF command switch.
const GPIO_pin_t GPIO_CMD_OFF = (GPIO_pin_t) { GPIOB, 1, 13, 0 };
const GPIO_pin_t GPIO_CMD_ON = (GPIO_pin_t) { GPIOB, 1, 14, 0 };
// Rotary encoder.
const GPIO_pin_t GPIO_ENC_SW = (GPIO_pin_t) { GPIOA, 0, 0, 0 };
const GPIO_pin_t GPIO_ENC_CHA = (GPIO_pin_t) { GPIOA, 0, 2, 0 };
const GPIO_pin_t GPIO_ENC_CHB = (GPIO_pin_t) { GPIOA, 0, 3, 0 };
// RGB LED.
const TIM_gpio_t GPIO_LED_TIM = { (const TIM_channel_gpio_t**) &GPIO_TIM_PINS_LIST, GPIO_TIM_CHANNEL_INDEX_LAST };
// Test points.
const GPIO_pin_t GPIO_TP1 = (GPIO_pin_t) { GPIOB, 1, 4, 0 };
const GPIO_pin_t GPIO_TP2 = (GPIO_pin_t) { GPIOB, 1, 5, 0 };
const GPIO_pin_t GPIO_TP3 = (GPIO_pin_t) { GPIOA, 0, 11, 0 };
