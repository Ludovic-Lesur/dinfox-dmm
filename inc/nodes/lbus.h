/*
 * lbus.h
 *
 *  Created on: 28 oct. 2022
 *      Author: Ludo
 */

#ifndef __LBUS_H__
#define __LBUS_H__

#include "node.h"
#include "lpuart.h"
#include "types.h"

/*** LBUS macros ***/

#define LBUS_BAUD_RATE		1200
#define LBUS_ADDRESS_MASK	0x7F
#define LBUS_ADDRESS_LAST	LBUS_ADDRESS_MASK

/*** LBUS functions ***/

void LBUS_init(void);
NODE_status_t LBUS_send(NODE_address_t destination_address, uint8_t* data, uint32_t data_size_bytes);
void LBUS_reset(void);
void LBUS_fill_rx_buffer(uint8_t rx_byte);

#define LBUS_status_check(error_base) { if (lbus_status != LBUS_SUCCESS) { status = error_base + lbus_status; goto errors; }}
#define LBUS_error_check() { ERROR_status_check(lbus_status, LBUS_SUCCESS, ERROR_BASE_LBUS); }
#define LBUS_error_check_print() { ERROR_status_check_print(lbus_status, LBUS_SUCCESS, ERROR_BASE_LBUS); }

#endif /* __LBUS_H__ */
