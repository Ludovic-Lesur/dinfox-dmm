/*
 * main.c
 *
 *  Created on: Jan 7, 2023
 *      Author: Ludo
 */

// Registers
#include "rcc_reg.h"
// Peripherals.
#include "adc.h"
#include "exti.h"
#include "flash.h"
#include "gpio.h"
#include "i2c.h"
#include "iwdg.h"
#include "lptim.h"
#include "lpuart.h"
#include "mapping.h"
#include "nvic.h"
#include "pwr.h"
#include "rcc.h"
#include "rtc.h"
// Components.
#include "led.h"
// Utils.
#include "types.h"
// Applicative.
#include "rs485.h"
#include "error.h"
#include "mode.h"
#include "version.h"

/* COMMON INIT FUNCTION FOR MAIN CONTEXT.
 * @param:	None.
 * @return:	None.
 */
void _DMM_init_context(void) {

}

/* COMMON INIT FUNCTION FOR PERIPHERALS AND COMPONENTS.
 * @param:	None.
 * @return:	None.
 */
void _DMM_init_hw(void) {

}

/* MAIN FUNCTION.
 * @param:	None.
 * @return:	None.
 */
int main(void) {
	return 0;
}
