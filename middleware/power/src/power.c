/*
 * power.c
 *
 *  Created on: 22 jul. 2023
 *      Author: Ludo
 */

#include "power.h"

#include "analog.h"
#include "error.h"
#include "error_base.h"
#include "gpio.h"
#include "gpio_mapping.h"
#include "hmi.h"
#include "lptim.h"
#include "types.h"

/*** POWER local global variables ***/

static uint32_t power_domain_state[POWER_DOMAIN_LAST];

/*** POWER functions ***/

/*******************************************************************/
void POWER_init(void) {
    // Local variables.
    uint8_t idx = 0;
    // Init context.
    for (idx = 0; idx < POWER_DOMAIN_LAST; idx++) {
        power_domain_state[idx] = 0;
    }
	// Init power control pins.
	GPIO_configure(&GPIO_MNTR_EN, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_HMI_POWER_ENABLE, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_RS485_POWER_ENABLE, GPIO_MODE_OUTPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
}

/*******************************************************************/
POWER_status_t POWER_enable(POWER_requester_id_t requester_id, POWER_domain_t domain, LPTIM_delay_mode_t delay_mode) {
	// Local variables.
	POWER_status_t status = POWER_SUCCESS;
	ANALOG_status_t analog_status = ANALOG_SUCCESS;
	HMI_status_t hmi_status = HMI_SUCCESS;
	LPTIM_status_t lptim_status = LPTIM_SUCCESS;
	uint32_t delay_ms = 0;
	uint8_t action_required = 0;
	// Check parameters.
	if (requester_id >= POWER_REQUESTER_ID_LAST) {
	    status = POWER_ERROR_REQUESTER_ID;
        goto errors;
	}
	if (domain >= POWER_DOMAIN_LAST) {
	    status = POWER_ERROR_DOMAIN;
        goto errors;
	}
	action_required = ((power_domain_state[domain] == 0) ? 1 : 0);
	// Update state.
	power_domain_state[domain] |= (0b1 << requester_id);
	// Directly exit if this is not the first request.
	if (action_required == 0) goto errors;
	// Check domain.
	switch (domain) {
	case POWER_DOMAIN_ANALOG:
		// Enable voltage dividers and init ADC.
		GPIO_write(&GPIO_MNTR_EN, 1);
		analog_status = ANALOG_init();
		ANALOG_stack_exit_error(ERROR_BASE_ANALOG, POWER_ERROR_DRIVER_ANALOG);
		delay_ms = POWER_ON_DELAY_MS_ANALOG;
		break;
	case POWER_DOMAIN_HMI:
		// Turn HMI on and init drivers.
		GPIO_write(&GPIO_HMI_POWER_ENABLE, 1);
		hmi_status = HMI_init();
		HMI_stack_exit_error(ERROR_BASE_HMI, POWER_ERROR_DRIVER_HMI);
		delay_ms = POWER_ON_DELAY_MS_HMI;
		break;
	case POWER_DOMAIN_RS485:
		// Turn RS485 transceiver on.
		GPIO_write(&GPIO_RS485_POWER_ENABLE, 1);
		delay_ms = POWER_ON_DELAY_MS_RS485;
		break;
	default:
		status = POWER_ERROR_DOMAIN;
		goto errors;
	}
	// Power on delay.
	if (delay_ms != 0) {
		lptim_status = LPTIM_delay_milliseconds(delay_ms, delay_mode);
		LPTIM_stack_exit_error(ERROR_BASE_LPTIM, POWER_ERROR_DRIVER_LPTIM);
	}
errors:
	return status;
}

/*******************************************************************/
POWER_status_t POWER_disable(POWER_requester_id_t requester_id, POWER_domain_t domain) {
	// Local variables.
	POWER_status_t status = POWER_SUCCESS;
	ANALOG_status_t analog_status = ANALOG_SUCCESS;
	HMI_status_t hmi_status = HMI_SUCCESS;
	// Check parameters.
    if (requester_id >= POWER_REQUESTER_ID_LAST) {
        status = POWER_ERROR_REQUESTER_ID;
        goto errors;
    }
    if (domain >= POWER_DOMAIN_LAST) {
        status = POWER_ERROR_DOMAIN;
        goto errors;
    }
    if (power_domain_state[domain] == 0) goto errors;
    // Update state.
    power_domain_state[domain] &= ~(0b1 << requester_id);
    // Directly exit if this is not the last request.
    if (power_domain_state[domain] != 0) goto errors;
	// Check domain.
	switch (domain) {
	case POWER_DOMAIN_ANALOG:
		// Disable voltage dividers and release ADC.
		analog_status = ANALOG_de_init();
		GPIO_write(&GPIO_MNTR_EN, 0);
		ANALOG_stack_exit_error(ERROR_BASE_ANALOG, POWER_ERROR_DRIVER_ANALOG);
		break;
	case POWER_DOMAIN_HMI:
		// Turn HMI off and release drivers.
	    hmi_status = HMI_de_init();
	    GPIO_write(&GPIO_HMI_POWER_ENABLE, 0);
		HMI_stack_exit_error(ERROR_BASE_HMI, POWER_ERROR_DRIVER_HMI);
		break;
	case POWER_DOMAIN_RS485:
		// Turn RS485 transceiver off.
		GPIO_write(&GPIO_RS485_POWER_ENABLE, 0);
		break;
	default:
		status = POWER_ERROR_DOMAIN;
		goto errors;
	}
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
