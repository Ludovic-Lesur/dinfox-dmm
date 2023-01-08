/*
 * lpuart.h
 *
 *  Created on: 25 oct. 2022
 *      Author: Ludo
 */

#ifndef __LPUART_H__
#define __LPUART_H__

#include "mode.h"
#include "rs485_common.h"
#include "types.h"

/*** LPUART structures ***/

typedef enum {
	LPUART_SUCCESS = 0,
	LPUART_ERROR_NULL_PARAMETER,
	LPUART_ERROR_MODE,
	LPUART_ERROR_NODE_ADDRESS,
	LPUART_ERROR_TX_TIMEOUT,
	LPUART_ERROR_TC_TIMEOUT,
	LPUART_ERROR_STRING_SIZE,
	LPUART_ERROR_BASE_LAST = 0x0100
} LPUART_status_t;

/*** LPUART functions ***/

#ifdef AM
LPUART_status_t LPUART1_init(RS485_address_t node_address);
#else
void LPUART1_init(void);
#endif
void LPUART1_enable_rx(void);
void LPUART1_disable_rx(void);
#ifdef AM
LPUART_status_t LPUART1_send_command(RS485_address_t slave_address, char_t* command);
#else
LPUART_status_t LPUART1_send_command(char_t* command);
#endif

#define LPUART1_status_check(error_base) { if (lpuart1_status != LPUART_SUCCESS) { status = error_base + lpuart1_status; goto errors; }}
#define LPUART1_error_check() { ERROR_status_check(lpuart1_status, LPUART_SUCCESS, ERROR_BASE_LPUART1); }
#define LPUART1_error_check_print() { ERROR_status_check_print(lpuart1_status, LPUART_SUCCESS, ERROR_BASE_LPUART1); }

#endif /* __LPUART_H__ */
