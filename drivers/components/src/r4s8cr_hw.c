/*
 * r4s8cr_hw.c
 *
 *  Created on: 23 dec. 2024
 *      Author: Ludo
 */

#include "r4s8cr_hw.h"

#ifndef R4S8CR_DRIVER_DISABLE_FLAGS_FILE
#include "r4s8cr_driver_flags.h"
#endif
#include "error.h"
#include "lpuart.h"
#include "gpio_mapping.h"
#include "iwdg.h"
#include "nvic_priority.h"
#include "r4s8cr.h"
#include "types.h"
#include "una.h"

#ifndef R4S8CR_DRIVER_DISABLE

/*** R4S8CR HW functions ***/

/*******************************************************************/
R4S8CR_status_t R4S8CR_HW_init(R4S8CR_HW_configuration_t* configuration) {
    // Local variables.
    R4S8CR_status_t status = R4S8CR_SUCCESS;
    LPUART_status_t lpuart_status = LPUART_SUCCESS;
    LPUART_configuration_t lpuart_config;
    // Init LPUART.
    lpuart_config.baud_rate = (configuration->uart_baud_rate);
    lpuart_config.nvic_priority = NVIC_PRIORITY_RS485;
    lpuart_config.rxne_callback = (configuration->rx_irq_callback);
    lpuart_config.self_address = UNA_NODE_ADDRESS_MASTER;
    lpuart_config.rx_mode = LPUART_RX_MODE_DIRECT;
    lpuart_status = LPUART_init(&GPIO_RS485_LPUART, &lpuart_config);
    LPUART_exit_error(R4S8CR_ERROR_BASE_RS485);
errors:
    return status;
}

/*******************************************************************/
R4S8CR_status_t R4S8CR_HW_de_init(void) {
    // Local variables.
    R4S8CR_status_t status = R4S8CR_SUCCESS;
    LPUART_status_t lpuart_status = LPUART_SUCCESS;
    // Release LPUART.
    lpuart_status = LPUART_de_init(&GPIO_RS485_LPUART);
    LPUART_exit_error(R4S8CR_ERROR_BASE_RS485);
errors:
    return status;
}

/*******************************************************************/
R4S8CR_status_t R4S8CR_HW_enable_rx(void) {
    // Local variables.
    R4S8CR_status_t status = R4S8CR_SUCCESS;
    LPUART_status_t lpuart_status = LPUART_SUCCESS;
    // Enable receiver.
    lpuart_status = LPUART_enable_rx();
    LPUART_exit_error(R4S8CR_ERROR_BASE_RS485);
errors:
    return status;
}

/*******************************************************************/
R4S8CR_status_t R4S8CR_HW_disable_rx(void) {
    // Local variables.
    R4S8CR_status_t status = R4S8CR_SUCCESS;
    LPUART_status_t lpuart_status = LPUART_SUCCESS;
    // Disable receiver.
    lpuart_status = LPUART_disable_rx();
    LPUART_exit_error(R4S8CR_ERROR_BASE_RS485);
errors:
    return status;
}

/*******************************************************************/
R4S8CR_status_t R4S8CR_HW_write(uint8_t* data, uint32_t data_size_bytes) {
    // Local variables.
    R4S8CR_status_t status = R4S8CR_SUCCESS;
    LPUART_status_t lpuart_status = LPUART_SUCCESS;
    // Send bytes over LPUART.
    lpuart_status = LPUART_write(data, data_size_bytes);
    LPUART_exit_error(R4S8CR_ERROR_BASE_RS485);
errors:
    return status;
    return status;
}

/*******************************************************************/
R4S8CR_status_t R4S8CR_HW_delay_milliseconds(uint32_t delay_ms) {
    // Local variables.
    R4S8CR_status_t status = R4S8CR_SUCCESS;
    LPTIM_status_t lptim_status = LPTIM_SUCCESS;
    // Reload watchdog.
    IWDG_reload();
    // Perform delay.
    lptim_status = LPTIM_delay_milliseconds(delay_ms, LPTIM_DELAY_MODE_SLEEP);
    LPTIM_exit_error(R4S8CR_ERROR_BASE_DELAY);
errors:
    return status;
}

#endif /* R4S8CR_DRIVER_DISABLE */

