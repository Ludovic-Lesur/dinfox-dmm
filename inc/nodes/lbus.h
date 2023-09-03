/*
 * lbus.h
 *
 *  Created on: 28 oct. 2022
 *      Author: Ludo
 */

#ifndef __LBUS_H__
#define __LBUS_H__

#include "lpuart.h"
#include "node_common.h"
#include "types.h"

/*** LBUS macros ***/

#define LBUS_ADDRESS_MASK	0x7F
#define LBUS_ADDRESS_LAST	LBUS_ADDRESS_MASK

/*** LBUS structures ***/

/*!******************************************************************
 * \enum LBUS_status_t
 * \brief LBUS driver error codes.
 *******************************************************************/
typedef enum {
	// Driver errors.
	LBUS_SUCCESS = 0,
	LBUS_ERROR_ADDRESS,
	// Low level drivers errors.
	LBUS_ERROR_BASE_LPUART = 0x0100,
	// Last base value.
	LBUS_ERROR_BASE_LAST = (LBUS_ERROR_BASE_LPUART + LPUART_ERROR_BASE_LAST),
} LBUS_status_t;

/*!******************************************************************
 * \fn LBUS_rx_irq_cb
 * \brief LBUS RX interrupt callback.
 *******************************************************************/
typedef void (*LBUS_rx_irq_cb)(uint8_t data);

/*** LBUS functions ***/

/*!******************************************************************
 * \fn void LBUS_init(LBUS_rx_irq_cb irq_callback)
 * \brief Init LBUS interface.
 * \param[in]  	irq_callback: Function to call on frame reception interrupt.
 * \param[out] 	none
 * \retval		none
 *******************************************************************/
void LBUS_init(LBUS_rx_irq_cb irq_callback);

/*!******************************************************************
 * \fn LBUS_status_t LBUS_send(NODE_address_t destination_address, uint8_t* data, uint32_t data_size_bytes)
 * \brief Send data over LBUS interface.
 * \param[in]	destination_address: RS485 address of the destination node.
 * \param[in]	data: Byte array to send.
 * \param[in]	data_size_bytes: Number of bytes to send.
 * \param[out] 	none
 * \retval		Function execution status.
 *******************************************************************/
LBUS_status_t LBUS_send(NODE_address_t destination_address, uint8_t* data, uint32_t data_size_bytes);

/*!******************************************************************
 * \fn void LBUS_reset(void)
 * \brief Reset LBUS interface.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		none
 *******************************************************************/
void LBUS_reset(void);

/*******************************************************************/
#define LBUS_exit_error(error_base) { if (lbus_status != LBUS_SUCCESS) { status = (error_base + lbus_status); goto errors; } }

/*******************************************************************/
#define LBUS_stack_error(void) { if (lbus_status != LBUS_SUCCESS) { ERROR_stack_add(ERROR_BASE_LBUS + lbus_status); } }

/*******************************************************************/
#define LBUS_stack_exit_error(error_code) { if (lbus_status != LBUS_SUCCESS) { ERROR_stack_add(ERROR_BASE_LBUS + lbus_status); status = error_code; goto errors; } }

#endif /* __LBUS_H__ */
