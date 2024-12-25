/*
 * lmac_hw.c
 *
 *  Created on: 27 nov. 2024
 *      Author: Ludo
 */

#include "lmac_hw.h"

#ifndef LMAC_DRIVER_DISABLE_FLAGS_FILE
#include "lmac_driver_flags.h"
#endif
#include "error.h"
#include "gpio_mapping.h"
#include "lmac.h"
#include "nvic_priority.h"
#include "rtc.h"
#include "types.h"
#include "una.h"

#ifndef LMAC_DRIVER_DISABLE

/*** LMAC HW functions ***/

/*******************************************************************/
LMAC_status_t LMAC_HW_init(uint32_t baud_rate, LMAC_rx_irq_cb_t rx_irq_callback, uint8_t* self_address) {
    // Local variables.
    LMAC_status_t status = LMAC_SUCCESS;
    LPUART_status_t lpuart_status = LPUART_SUCCESS;
    LPUART_configuration_t lpuart_config;
    // Update self address.
    (*self_address) = UNA_NODE_ADDRESS_MASTER;
    // Init LPUART.
    lpuart_config.baud_rate = baud_rate;
    lpuart_config.nvic_priority = NVIC_PRIORITY_RS485;
    lpuart_config.rxne_callback = rx_irq_callback;
    lpuart_config.self_address = UNA_NODE_ADDRESS_MASTER;
    lpuart_config.rx_mode = LPUART_RX_MODE_ADDRESSED;
    lpuart_status = LPUART_init(&GPIO_RS485_LPUART, &lpuart_config);
    LPUART_exit_error(LMAC_ERROR_BASE_HW_INTERFACE);
errors:
    return status;
}

/*******************************************************************/
LMAC_status_t LMAC_HW_de_init(void) {
    // Local variables.
    LMAC_status_t status = LMAC_SUCCESS;
    LPUART_status_t lpuart_status = LPUART_SUCCESS;
    // Release LPUART.
    lpuart_status = LPUART_de_init(&GPIO_RS485_LPUART);
    LPUART_exit_error(LMAC_ERROR_BASE_HW_INTERFACE);
errors:
    return status;
}

/*******************************************************************/
LMAC_status_t LMAC_HW_enable_rx(void) {
    // Local variables.
    LMAC_status_t status = LMAC_SUCCESS;
    LPUART_status_t lpuart_status = LPUART_SUCCESS;
    // Enable receiver.
    lpuart_status = LPUART_enable_rx();
    LPUART_exit_error(LMAC_ERROR_BASE_HW_INTERFACE);
errors:
    return status;
}

/*******************************************************************/
LMAC_status_t LMAC_HW_disable_rx(void) {
    // Local variables.
    LMAC_status_t status = LMAC_SUCCESS;
    LPUART_status_t lpuart_status = LPUART_SUCCESS;
    // Disable receiver.
    lpuart_status = LPUART_disable_rx();
    LPUART_exit_error(LMAC_ERROR_BASE_HW_INTERFACE);
errors:
    return status;
}

/*******************************************************************/
LMAC_status_t LMAC_HW_write(uint8_t* data, uint32_t data_size_bytes) {
    // Local variables.
    LMAC_status_t status = LMAC_SUCCESS;
    LPUART_status_t lpuart_status = LPUART_SUCCESS;
    // Send bytes over LPUART.
    lpuart_status = LPUART_write(data, data_size_bytes);
    LPUART_exit_error(LMAC_ERROR_BASE_HW_INTERFACE);
errors:
    return status;
}

#ifdef LMAC_DRIVER_MODE_SLAVE
/*******************************************************************/
uint32_t LMAC_HW_get_uptime_seconds(void) {
    return RTC_get_uptime_seconds();
}
#endif

#endif /* LMAC_DRIVER_DISABLE */
