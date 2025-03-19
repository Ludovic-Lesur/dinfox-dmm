/*
 * mcu_mapping.c
 *
 *  Created on: 17 apr. 2024
 *      Author: Ludo
 */

#include "mcu_mapping.h"

#include "adc.h"
#include "gpio.h"
#include "gpio_registers.h"
#include "i2c.h"
#include "lpuart.h"
#include "tim.h"

/*** MCU MAPPING local global variables ***/

// Analog inputs.
static const GPIO_pin_t GPIO_ADC_VRS_MEASURE = { GPIOA, 0, 6, 0 };
static const GPIO_pin_t GPIO_ADC_VHMI_MEASURE = { GPIOA, 0, 4, 0 };
static const GPIO_pin_t GPIO_ADC_VUSB_MEASURE = { GPIOA, 0, 1, 0 };
// Analog inputs list.
static const GPIO_pin_t* const GPIO_ADC_PINS_LIST[ADC_CHANNEL_INDEX_LAST] = { &GPIO_ADC_VRS_MEASURE, &GPIO_ADC_VHMI_MEASURE, &GPIO_ADC_VUSB_MEASURE };
// LPUART1.
static const GPIO_pin_t GPIO_LPUART1_TX = { GPIOB, 1, 10, 4 };
static const GPIO_pin_t GPIO_LPUART1_RX = { GPIOB, 1, 11, 4 };
static const GPIO_pin_t GPIO_LPUART1_DE = { GPIOB, 1, 12, 2 };
static const GPIO_pin_t GPIO_LPUART1_NRE = { GPIOB, 1, 2, 0 };
// I2C1.
static const GPIO_pin_t GPIO_I2C1_SCL = { GPIOB, 1, 6, 1 };
static const GPIO_pin_t GPIO_I2C1_SDA = { GPIOB, 1, 7, 1 };
// RGB LED.
static const GPIO_pin_t GPIO_LED_RED = { GPIOA, 0, 7, 2 };
static const GPIO_pin_t GPIO_LED_GREEN = { GPIOB, 1, 0, 2 };
static const GPIO_pin_t GPIO_LED_BLUE = { GPIOB, 1, 1, 2 };
// Timer channels.
static const TIM_channel_gpio_t TIM_CHANNEL_GPIO_LED_RED = { TIM_CHANNEL_2, &GPIO_LED_RED, TIM_POLARITY_ACTIVE_LOW };
static const TIM_channel_gpio_t TIM_CHANNEL_GPIO_LED_GREEN = { TIM_CHANNEL_3, &GPIO_LED_GREEN, TIM_POLARITY_ACTIVE_LOW };
static const TIM_channel_gpio_t TIM_CHANNEL_GPIO_LED_BLUE = { TIM_CHANNEL_4, &GPIO_LED_BLUE, TIM_POLARITY_ACTIVE_LOW };
// Timer pins list.
static const TIM_channel_gpio_t* const TIM_CHANNEL_GPIO_LIST_LED[TIM_CHANNEL_INDEX_LED_LAST] = { &TIM_CHANNEL_GPIO_LED_RED, &TIM_CHANNEL_GPIO_LED_GREEN, &TIM_CHANNEL_GPIO_LED_BLUE };

/*** MCU MAPPING global variables ***/

// Analog inputs.
const GPIO_pin_t GPIO_MNTR_EN = { GPIOA, 0, 5, 0 };
const ADC_gpio_t ADC_GPIO = { (const GPIO_pin_t**) &GPIO_ADC_PINS_LIST, ADC_CHANNEL_INDEX_LAST };
// RS485 interface.
const GPIO_pin_t GPIO_RS485_POWER_ENABLE = { GPIOA, 0, 8, 0 };
const LPUART_gpio_t LPUART_GPIO_RS485 = { &GPIO_LPUART1_TX, &GPIO_LPUART1_RX, &GPIO_LPUART1_DE, &GPIO_LPUART1_NRE };
// HMI.
const GPIO_pin_t GPIO_HMI_POWER_ENABLE = { GPIOC, 2, 13, 0 };
const I2C_gpio_t I2C_GPIO_HMI = { &GPIO_I2C1_SCL, &GPIO_I2C1_SDA };
// Push buttons.
const GPIO_pin_t GPIO_BP1 = { GPIOB, 1, 8, 0 };
const GPIO_pin_t GPIO_BP2 = { GPIOB, 1, 15, 0 };
const GPIO_pin_t GPIO_BP3 = { GPIOB, 1, 9, 0 };
// ON/OFF command switch.
const GPIO_pin_t GPIO_CMD_OFF = { GPIOB, 1, 13, 0 };
const GPIO_pin_t GPIO_CMD_ON = { GPIOB, 1, 14, 0 };
// Rotary encoder.
const GPIO_pin_t GPIO_ENC_SW = { GPIOA, 0, 0, 0 };
const GPIO_pin_t GPIO_ENC_CHA = { GPIOA, 0, 2, 0 };
const GPIO_pin_t GPIO_ENC_CHB = { GPIOA, 0, 3, 0 };
// RGB LED.
const TIM_gpio_t TIM_GPIO_LED = { (const TIM_channel_gpio_t**) &TIM_CHANNEL_GPIO_LIST_LED, TIM_CHANNEL_INDEX_LED_LAST };
// Test points.
const GPIO_pin_t GPIO_TP1 = { GPIOB, 1, 4, 0 };
const GPIO_pin_t GPIO_TP2 = { GPIOB, 1, 5, 0 };
const GPIO_pin_t GPIO_TP3 = { GPIOA, 0, 11, 0 };
