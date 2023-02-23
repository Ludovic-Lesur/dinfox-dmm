/*
 * exti.c
 *
 *  Created on: 18 apr. 2020
 *      Author: Ludo
 */

#include "exti.h"

#include "exti_reg.h"
#include "gpio.h"
#include "hmi.h"
#include "mapping.h"
#include "mode.h"
#include "nvic.h"
#include "rcc_reg.h"
#include "syscfg_reg.h"
#include "types.h"

/*** EXTI local macros ***/

#define EXTI_RTSR_FTSR_RESERVED_INDEX	18
#define EXTI_RTSR_FTSR_MAX_INDEX		22

/*** EXTI local global variables ***/

static volatile uint8_t encoder_switch_flag = 0;

/*** EXTI local functions ***/

/* EXTI LINES 0-1 INTERRUPT HANDLER.
 * @param:	None.
 * @return:	None.
 */
void __attribute__((optimize("-O0"))) EXTI0_1_IRQHandler(void) {
	// Rotary encoder switch IRQ (PA0).
	if (((EXTI -> PR) & (0b1 << (GPIO_ENC_SW.pin_index))) != 0) {
		// Set flag in HMI driver.
		HMI_set_irq_flag(HMI_IRQ_ENCODER_SWITCH);
		encoder_switch_flag = 1;
		// Clear flag.
		EXTI -> PR |= (0b1 << (GPIO_ENC_SW.pin_index));
	}
}

/* EXTI LINES 2-3 INTERRUPT HANDLER.
 * @param:	None.
 * @return:	None.
 */
void __attribute__((optimize("-O0"))) EXTI2_3_IRQHandler(void) {
	// Rotary encoder channel A (PA2).
	if (((EXTI -> PR) & (0b1 << (GPIO_ENC_CHA.pin_index))) != 0) {
		// Check channel B state.
		if (GPIO_read(&GPIO_ENC_CHB) == 0) {
			// Set flag in HMI driver.
			HMI_set_irq_flag(HMI_IRQ_ENCODER_FORWARD);
		}
		// Clear flag.
		EXTI -> PR |= (0b1 << (GPIO_ENC_CHA.pin_index));
	}
	// Rotary encoder channel B (PA3).
	if (((EXTI -> PR) & (0b1 << (GPIO_ENC_CHB.pin_index))) != 0) {
		// Check channel A state.
		if (GPIO_read(&GPIO_ENC_CHA) == 0) {
			// Set flag in HMI driver.
			HMI_set_irq_flag(HMI_IRQ_ENCODER_BACKWARD);
		}
		// Clear flag.
		EXTI -> PR |= (0b1 << (GPIO_ENC_CHB.pin_index));
	}
}

/* EXTI LINES 4-15 INTERRUPT HANDLER.
 * @param:	None.
 * @return:	None.
 */
void __attribute__((optimize("-O0"))) EXTI4_15_IRQHandler(void) {
	// BP1 (PB8).
	if (((EXTI -> PR) & (0b1 << (GPIO_BP1.pin_index))) != 0) {
		// Set flag in HMI driver.
		HMI_set_irq_flag(HMI_IRQ_BP1);
		// Clear flag.
		EXTI -> PR |= (0b1 << (GPIO_BP1.pin_index));
	}
	// BP2 (PB15).
	if (((EXTI -> PR) & (0b1 << (GPIO_BP2.pin_index))) != 0) {
		// Set flag in HMI driver.
		HMI_set_irq_flag(HMI_IRQ_BP2);
		// Clear flag.
		EXTI -> PR |= (0b1 << (GPIO_BP2.pin_index));
	}
	// BP3 (PB9).
	if (((EXTI -> PR) & (0b1 << (GPIO_BP3.pin_index))) != 0) {
		// Set flag in HMI driver.
		HMI_set_irq_flag(HMI_IRQ_BP3);
		// Clear flag.
		EXTI -> PR |= (0b1 << (GPIO_BP3.pin_index));
	}
	// CMD_ON (PB13).
	if (((EXTI -> PR) & (0b1 << (GPIO_CMD_ON.pin_index))) != 0) {
		// Set flag in HMI driver.
		HMI_set_irq_flag(HMI_IRQ_CMD_ON);
		// Clear flag.
		EXTI -> PR |= (0b1 << (GPIO_CMD_ON.pin_index));
	}
	// CMD_OFF (PB14).
	if (((EXTI -> PR) & (0b1 << (GPIO_CMD_OFF.pin_index))) != 0) {
		// Set flag in HMI driver.
		HMI_set_irq_flag(HMI_IRQ_CMD_OFF);
		// Clear flag.
		EXTI -> PR |= (0b1 << (GPIO_CMD_OFF.pin_index));
	}
}

/* SET EXTI TRIGGER.
 * @param trigger:	Interrupt edge trigger (see EXTI_trigger_t enum).
 * @param line_idx:	Line index.
 * @return:			None.
 */
static void _EXTI_set_trigger(EXTI_trigger_t trigger, uint8_t line_idx) {
	// Select triggers.
	switch (trigger) {
	// Rising edge only.
	case EXTI_TRIGGER_RISING_EDGE:
		EXTI -> RTSR |= (0b1 << line_idx); // Rising edge enabled.
		EXTI -> FTSR &= ~(0b1 << line_idx); // Falling edge disabled.
		break;
	// Falling edge only.
	case EXTI_TRIGGER_FALLING_EDGE:
		EXTI -> RTSR &= ~(0b1 << line_idx); // Rising edge disabled.
		EXTI -> FTSR |= (0b1 << line_idx); // Falling edge enabled.
		break;
	// Both edges.
	case EXTI_TRIGGER_ANY_EDGE:
		EXTI -> RTSR |= (0b1 << line_idx); // Rising edge enabled.
		EXTI -> FTSR |= (0b1 << line_idx); // Falling edge enabled.
		break;
	// Unknown configuration.
	default:
		break;
	}
	// Clear flag.
	EXTI -> PR |= (0b1 << line_idx);
}

/*** EXTI functions ***/

/* INIT EXTI PERIPHERAL.
 * @param:	None.
 * @return:	None.
 */
void EXTI_init(void) {
	// Enable peripheral clock.
	RCC -> APB2ENR |= (0b1 << 0); // SYSCFEN='1'.
	// Mask all sources by default.
	EXTI -> IMR = 0;
	// Clear all flags.
	EXTI_clear_all_flags();
}

/* CONFIGURE A GPIO AS EXTERNAL INTERRUPT SOURCE.
 * @param gpio:		GPIO to be attached to EXTI peripheral.
 * @param trigger:	Interrupt edge trigger (see EXTI_trigger_t enum).
 * @return:			None.
 */
void EXTI_configure_gpio(const GPIO_pin_t* gpio, EXTI_trigger_t trigger) {
	// Select GPIO port.
	SYSCFG -> EXTICR[((gpio -> pin_index) / 4)] &= ~(0b1111 << (4 * ((gpio -> pin_index) % 4)));
	SYSCFG -> EXTICR[((gpio -> pin_index) / 4)] |= ((gpio -> port_index) << (4 * ((gpio -> pin_index) % 4)));
	// Set mask.
	EXTI -> IMR |= (0b1 << ((gpio -> pin_index))); // IMx='1'.
	// Select triggers.
	_EXTI_set_trigger(trigger, (gpio -> pin_index));
}

/* CONFIGURE A LINE AS INTERNAL INTERRUPT SOURCE.
 * @param line:		Line to configure (see EXTI_line_t enum).
 * @param trigger:	Interrupt edge trigger (see EXTI_trigger_t enum).
 * @return:			None.
 */
void EXTI_configure_line(EXTI_line_t line, EXTI_trigger_t trigger) {
	// Set mask.
	EXTI -> IMR |= (0b1 << line); // IMx='1'.
	// Select triggers.
	if ((line != EXTI_RTSR_FTSR_RESERVED_INDEX) || (line <= EXTI_RTSR_FTSR_MAX_INDEX)) {
		_EXTI_set_trigger(trigger, line);
	}
}

/* CLEAR EXTI FLAG.
 * @param line:	Line to clear (see EXTI_line_t enum).
 * @return:		None.
 */
void EXTI_clear_flag(EXTI_line_t line) {
	// Clear flag.
	EXTI -> PR |= line; // PIFx='1'.
}

/* CLEAR ALL EXTI FLAGS.
 * @param:	None.
 * @return:	None.
 */
void EXTI_clear_all_flags(void) {
	// Clear all flags.
	EXTI -> PR |= 0x007BFFFF; // PIFx='1'.
}

/* READ ENCODER SWITCH FLAG.
 * @param:	None.
 * @return:	None.
 */
uint8_t EXTI_get_encoder_switch_flag(void) {
	return encoder_switch_flag;
}

/* CLEAR ENCODER SWITCH FLAG.
 * @param:	None.
 * @return:	None.
 */
void EXTI_clear_encoder_switch_flag(void) {
	encoder_switch_flag = 0;
}
