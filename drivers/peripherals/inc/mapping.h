/*
 * mapping.h
 *
 *  Created on: 18 apr. 2020
 *      Author: Ludo
 */

#ifndef __MAPPING_H__
#define __MAPPING_H__

#include "gpio.h"

/*** MAPPING global variables ***/

// Rotary encoder.
extern const GPIO_pin_t GPIO_ENC_SW;
extern const GPIO_pin_t GPIO_ENC_CHA;
extern const GPIO_pin_t GPIO_ENC_CHB;
// Analog inputs.
extern const GPIO_pin_t GPIO_ADC1_IN1;
extern const GPIO_pin_t GPIO_ADC1_IN4;
extern const GPIO_pin_t GPIO_ADC1_IN6;
// Monitoring enable.
extern const GPIO_pin_t GPIO_MNTR_EN;
// RGB LED.
extern const GPIO_pin_t GPIO_LED_RED;
extern const GPIO_pin_t GPIO_LED_GREEN;
extern const GPIO_pin_t GPIO_LED_BLUE;
// LPUART1 (RS485).
extern const GPIO_pin_t GPIO_TRX_POWER_ENABLE;
extern const GPIO_pin_t GPIO_LPUART1_TX;
extern const GPIO_pin_t GPIO_LPUART1_RX;
extern const GPIO_pin_t GPIO_LPUART1_DE;
extern const GPIO_pin_t GPIO_LPUART1_NRE;
// USART1
extern const GPIO_pin_t GPIO_USART1_TX;
extern const GPIO_pin_t GPIO_USART1_RX;
// Test points.
extern const GPIO_pin_t GPIO_TP1;
extern const GPIO_pin_t GPIO_TP2;
extern const GPIO_pin_t GPIO_TP3;
// I2C1 (OLED screen).
extern const GPIO_pin_t GPIO_HMI_POWER_ENABLE;
extern const GPIO_pin_t GPIO_I2C1_SCL;
extern const GPIO_pin_t GPIO_I2C1_SDA;
// Push buttons.
extern const GPIO_pin_t GPIO_BP1;
extern const GPIO_pin_t GPIO_BP2;
extern const GPIO_pin_t GPIO_BP3;
// ON/OFF command switch.
extern const GPIO_pin_t GPIO_CMD_OFF;
extern const GPIO_pin_t GPIO_CMD_ON;
// Programming.
extern const GPIO_pin_t GPIO_SWDIO;
extern const GPIO_pin_t GPIO_SWCLK;

#endif /* __MAPPING_H__ */
