/*
 * lpuart.h
 *
 *  Created on: 25 oct. 2022
 *      Author: Ludo
 */

#ifndef __LPUART_H__
#define __LPUART_H__

#include "lptim.h"
#include "mode.h"
#include "node_common.h"
#include "types.h"

/*** LPUART structures ***/

typedef void (*LPUART_rx_callback_t)(uint8_t rx_byte);

typedef enum {
	LPUART_SUCCESS = 0,
	LPUART_ERROR_NULL_PARAMETER,
	LPUART_ERROR_MODE,
	LPUART_ERROR_NODE_ADDRESS,
	LPUART_ERROR_TX_TIMEOUT,
	LPUART_ERROR_TC_TIMEOUT,
	LPUART_ERROR_BASE_LPTIM = 0x0100,
	LPUART_ERROR_BASE_LAST = (LPUART_ERROR_BASE_LPTIM + LPTIM_ERROR_BASE_LAST)
} LPUART_status_t;

/*** LPUART functions ***/

#ifdef AM
void LPUART1_init(NODE_address_t self_address);
#else
void LPUART1_init(void);
#endif
LPUART_status_t LPUART1_power_on(void);
void LPUART1_power_off(void);
void LPUART1_enable_rx(void);
void LPUART1_disable_rx(void);
LPUART_status_t LPUART1_send(uint8_t* data, uint8_t data_size_bytes, LPUART_rx_callback_t rx_callback);

#define LPUART1_status_check(error_base) { if (lpuart1_status != LPUART_SUCCESS) { status = error_base + lpuart1_status; goto errors; }}
#define LPUART1_error_check() { ERROR_status_check(lpuart1_status, LPUART_SUCCESS, ERROR_BASE_LPUART1); }
#define LPUART1_error_check_print() { ERROR_status_check_print(lpuart1_status, LPUART_SUCCESS, ERROR_BASE_LPUART1); }

#endif /* __LPUART_H__ */
