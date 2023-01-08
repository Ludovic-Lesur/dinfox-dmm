/*
 * mapping.h
 *
 *  Created on: 18 apr. 2020
 *      Author: Ludo
 */

#ifndef __MAPPING_H__
#define __MAPPING_H__

#include "gpio.h"
#include "gpio_reg.h"

// Rotary encoder.
static const GPIO_pin_t GPIO_ENC_SW =			(GPIO_pin_t) {GPIOA, 0, 0, 0};
static const GPIO_pin_t GPIO_ENC_CHA =			(GPIO_pin_t) {GPIOA, 0, 2, 0};
static const GPIO_pin_t GPIO_ENC_CHB =			(GPIO_pin_t) {GPIOA, 0, 3, 0};
// Analog inputs.
static const GPIO_pin_t GPIO_ADC1_IN1 =			(GPIO_pin_t) {GPIOA, 0, 1, 0};
static const GPIO_pin_t GPIO_ADC1_IN4 =			(GPIO_pin_t) {GPIOA, 0, 4, 0};
static const GPIO_pin_t GPIO_ADC1_IN6 =			(GPIO_pin_t) {GPIOA, 0, 6, 0};
// Monitoring enable.
static const GPIO_pin_t GPIO_MNTR_EN =			(GPIO_pin_t) {GPIOA, 0, 5, 0};
// RGB LED.
static const GPIO_pin_t GPIO_LED_RED =			(GPIO_pin_t) {GPIOA, 0, 7, 2}; // AF2 = TIM3_CH2.
static const GPIO_pin_t GPIO_LED_GREEN =		(GPIO_pin_t) {GPIOB, 1, 0, 2}; // AF2 = TIM3_CH3.
static const GPIO_pin_t GPIO_LED_BLUE =			(GPIO_pin_t) {GPIOB, 1, 1, 2}; // AF2 = TIM3_CH4.
// LPUART1 (RS485).
static const GPIO_pin_t GPIO_TRX_POWER_ENABLE =	(GPIO_pin_t) {GPIOA, 0, 8, 0};
static const GPIO_pin_t GPIO_LPUART1_TX =		(GPIO_pin_t) {GPIOB, 1, 10, 4}; // AF4 = LPUART1_TX.
static const GPIO_pin_t GPIO_LPUART1_RX =		(GPIO_pin_t) {GPIOB, 1, 11, 4}; // AF4 = LPUART1_RX.
static const GPIO_pin_t GPIO_LPUART1_DE =		(GPIO_pin_t) {GPIOB, 1, 12, 2}; // AF2 = LPUART1_DE.
static const GPIO_pin_t GPIO_LPUART1_NRE =		(GPIO_pin_t) {GPIOB, 1, 2, 0};
// USART1
static const GPIO_pin_t GPIO_USART1_TX =		(GPIO_pin_t) {GPIOA, 0, 9, 4}; // AF4 = USART1_TX.
static const GPIO_pin_t GPIO_USART1_RX =		(GPIO_pin_t) {GPIOA, 0, 10, 4}; // AF4 = USART1_RX.
// Test points.
static const GPIO_pin_t GPIO_TP1 =				(GPIO_pin_t) {GPIOB, 1, 4, 0};
static const GPIO_pin_t GPIO_TP2 =				(GPIO_pin_t) {GPIOB, 1, 5, 0};
static const GPIO_pin_t GPIO_TP3 =				(GPIO_pin_t) {GPIOA, 0, 11, 0};
// I2C1 (OLED screen).
static const GPIO_pin_t GPIO_HMI_POWER_ENABLE =	(GPIO_pin_t) {GPIOC, 2, 13, 0};
static const GPIO_pin_t GPIO_I2C1_SCL =			(GPIO_pin_t) {GPIOB, 1, 6, 1}; // AF1 = I2C1_SCL.
static const GPIO_pin_t GPIO_I2C1_SDA =			(GPIO_pin_t) {GPIOB, 1, 7, 1}; // AF1 = I2C1_SDA.
// Push buttons.
static const GPIO_pin_t GPIO_BP1 =				(GPIO_pin_t) {GPIOB, 1, 8, 0};
static const GPIO_pin_t GPIO_BP2 =				(GPIO_pin_t) {GPIOB, 1, 15, 0};
static const GPIO_pin_t GPIO_BP3 =				(GPIO_pin_t) {GPIOB, 1, 9, 0};
// ON/OFF command switch.
static const GPIO_pin_t GPIO_CMD_OFF =			(GPIO_pin_t) {GPIOB, 1, 13, 0};
static const GPIO_pin_t GPIO_CMD_ON =			(GPIO_pin_t) {GPIOB, 1, 14, 0};
// Programming.
static const GPIO_pin_t GPIO_SWDIO =			(GPIO_pin_t) {GPIOA, 0, 13, 0};
static const GPIO_pin_t GPIO_SWCLK =			(GPIO_pin_t) {GPIOA, 0, 14, 0};

#endif /* __MAPPING_H__ */
