/*
 * terminal_hw.c
 *
 *  Created on: 23 dec. 2024
 *      Author: Ludo
 */

#include "terminal_hw.h"

#ifndef EMBEDDED_UTILS_DISABLE_FLAGS_FILE
#include "embedded_utils_flags.h"
#endif
#include "error.h"
#include "error_base.h"
#include "lmac.h"
#include "terminal.h"
#include "terminal_instance.h"
#include "types.h"

#if (!(defined EMBEDDED_UTILS_TERMINAL_DRIVER_DISABLE) && (EMBEDDED_UTILS_TERMINAL_INSTANCES_NUMBER > 0))

/*** TERMINAL HW functions ***/

/*******************************************************************/
TERMINAL_status_t TERMINAL_HW_init(uint8_t instance, uint32_t baud_rate, TERMINAL_rx_irq_cb_t rx_irq_callback) {
    // Local variables.
    TERMINAL_status_t status = TERMINAL_SUCCESS;
    LMAC_status_t lmac_status = LMAC_SUCCESS;
    // Unused parameter.
    UNUSED(instance);
    // Init LMAC layer.
    lmac_status = LMAC_init(baud_rate, rx_irq_callback);
    LMAC_exit_error(TERMINAL_ERROR_BASE_HW_INTERFACE);
errors:
    return status;
}

/*******************************************************************/
TERMINAL_status_t TERMINAL_HW_de_init(uint8_t instance) {
    // Local variables.
    TERMINAL_status_t status = TERMINAL_SUCCESS;
    LMAC_status_t lmac_status = LMAC_SUCCESS;
    // Unused parameter.
    UNUSED(instance);
    // Release LMAC layer.
    lmac_status = LMAC_de_init();
    LMAC_stack_error(ERROR_BASE_TERMINAL + TERMINAL_ERROR_BASE_HW_INTERFACE);
    return status;
}

/*******************************************************************/
TERMINAL_status_t TERMINAL_HW_enable_rx(uint8_t instance) {
    // Local variables.
    TERMINAL_status_t status = TERMINAL_SUCCESS;
    LMAC_status_t lmac_status = LMAC_SUCCESS;
    // Unused parameter.
    UNUSED(instance);
    // Enable receiver.
    lmac_status = LMAC_enable_rx();
    LMAC_exit_error(TERMINAL_ERROR_BASE_HW_INTERFACE);
errors:
    return status;
}

/*******************************************************************/
TERMINAL_status_t TERMINAL_HW_disable_rx(uint8_t instance) {
    // Local variables.
    TERMINAL_status_t status = TERMINAL_SUCCESS;
    LMAC_status_t lmac_status = LMAC_SUCCESS;
    // Unused parameter.
    UNUSED(instance);
    // Disable receiver.
    lmac_status = LMAC_disable_rx();
    LMAC_exit_error(TERMINAL_ERROR_BASE_HW_INTERFACE);
errors:
    return status;
}

/*******************************************************************/
TERMINAL_status_t TERMINAL_HW_write(uint8_t instance, uint8_t* data, uint32_t data_size_bytes) {
    // Local variables.
    TERMINAL_status_t status = TERMINAL_SUCCESS;
    LMAC_status_t lmac_status = LMAC_SUCCESS;
    // Unused parameter.
    UNUSED(instance);
    // Write over LMAC layer.
    lmac_status = LMAC_write(data, data_size_bytes);
    LMAC_exit_error(TERMINAL_ERROR_BASE_HW_INTERFACE);
errors:
    return status;
}

#ifdef EMBEDDED_UTILS_TERMINAL_MODE_BUS
/*******************************************************************/
TERMINAL_status_t TERMINAL_HW_set_destination_address(uint8_t instance, uint8_t destination_address) {
    // Local variables.
    TERMINAL_status_t status = TERMINAL_SUCCESS;
    LMAC_status_t lmac_status = LMAC_SUCCESS;
    // Unused parameter.
    UNUSED(instance);
    // Set destination address.
    lmac_status = LMAC_set_destination_address(destination_address);
    LMAC_exit_error(TERMINAL_ERROR_BASE_HW_INTERFACE);
errors:
    return status;
}
#endif

#endif /* EMBEDDED_UTILS_TERMINAL_DRIVER_DISABLE */
