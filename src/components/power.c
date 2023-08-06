/*
 * power.c
 *
 *  Created on: 22 jul. 2023
 *      Author: Ludo
 */

#include "power.h"

#include "adc.h"
#include "gpio.h"
#include "hmi.h"
#include "lptim.h"
#include "mapping.h"
#include "node.h"
#include "types.h"

/*** POWER local global variables ***/

static uint8_t power_domain_state[POWER_DOMAIN_LAST];

/*** POWER functions ***/

/*******************************************************************/
void POWER_init(void) {
	// Local variables.
	POWER_domain_t domain = 0;
	// Init power control pins.
	GPIO_configure(&GPIO_MNTR_EN, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_HMI_POWER_ENABLE, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_TRX_POWER_ENABLE, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	// Disable all domains by default.
	for (domain=0 ; domain<POWER_DOMAIN_LAST ; domain++) {
		POWER_disable(domain);
	}
}

/*******************************************************************/
POWER_status_t POWER_enable(POWER_domain_t domain, LPTIM_delay_mode_t delay_mode) {
	// Local variables.
	POWER_status_t status = POWER_SUCCESS;
	ADC_status_t adc1_status = ADC_SUCCESS;
	LPTIM_status_t lptim1_status = LPTIM_SUCCESS;
	uint32_t delay_ms = 0;
	// Check domain.
	switch (domain) {
	case POWER_DOMAIN_ANALOG:
		// Enable voltage dividers and init ADC.
		GPIO_write(&GPIO_MNTR_EN, 1);
		adc1_status = ADC1_init();
		ADC1_check_status(POWER_ERROR_BASE_ADC);
		delay_ms = POWER_ON_DELAY_MS_ANALOG;
		break;
	case POWER_DOMAIN_HMI:
		// Turn HMI on and init drivers.
		GPIO_write(&GPIO_HMI_POWER_ENABLE, 1);
		I2C1_init();
		HMI_init();
		delay_ms = POWER_ON_DELAY_MS_HMI;
		break;
	case POWER_DOMAIN_RS485:
		// Turn RS485 transceiver on and init nodes interface.
		GPIO_write(&GPIO_TRX_POWER_ENABLE, 1);
		NODE_init();
		delay_ms = POWER_ON_DELAY_MS_RS485;
		break;
	default:
		status = POWER_ERROR_DOMAIN;
		goto errors;
	}
	// Update state.
	power_domain_state[domain] = 1;
	// Power on delay.
	if (delay_ms != 0) {
		lptim1_status = LPTIM1_delay_milliseconds(delay_ms, delay_mode);
		LPTIM1_check_status(POWER_ERROR_BASE_LPTIM);
	}
errors:
	return status;
}

/*******************************************************************/
POWER_status_t POWER_disable(POWER_domain_t domain) {
	// Local variables.
	POWER_status_t status = POWER_SUCCESS;
	ADC_status_t adc1_status = ADC_SUCCESS;
	// Check domain.
	switch (domain) {
	case POWER_DOMAIN_ANALOG:
		// Disable voltage dividers and release ADC.
		adc1_status = ADC1_de_init();
		GPIO_write(&GPIO_MNTR_EN, 0);
		ADC1_check_status(POWER_ERROR_BASE_ADC);
		break;
	case POWER_DOMAIN_HMI:
		// Turn HMI off and release drivers.
		HMI_de_init();
		I2C1_de_init();
		GPIO_write(&GPIO_HMI_POWER_ENABLE, 0);
		break;
	case POWER_DOMAIN_RS485:
		// Turn RS485 transceiver off and release nodes interface.
		NODE_de_init();
		GPIO_write(&GPIO_TRX_POWER_ENABLE, 0);
		break;
	default:
		status = POWER_ERROR_DOMAIN;
		goto errors;
	}
	// Update state.
	power_domain_state[domain] = 0;
errors:
	return status;
}

/*******************************************************************/
POWER_status_t POWER_get_state(POWER_domain_t domain, uint8_t* state) {
	// Local variables.
	POWER_status_t status = POWER_SUCCESS;
	// Check parameters.
	if (domain >= POWER_DOMAIN_LAST) {
		status = POWER_ERROR_DOMAIN;
		goto errors;
	}
	if (state == NULL) {
		status = POWER_ERROR_NULL_PARAMETER;
		goto errors;
	}
	(*state) = power_domain_state[domain];
errors:
	return status;
}
