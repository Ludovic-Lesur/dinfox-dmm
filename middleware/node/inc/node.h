/*
 * node.h
 *
 *  Created on: 22 jan 2023
 *      Author: Ludo
 */

#ifndef __NODE_H__
#define __NODE_H__

#include "analog.h"
#include "lptim.h"
#include "power.h"
#include "types.h"
#include "una.h"
#include "una_at.h"
#include "una_dmm.h"
#include "una_r4s8cr.h"

/*** NODES macros ***/

#define NODE_LIST_SIZE  32

/*** NODE structures ***/

/*!******************************************************************
 * \enum NODE_status_t
 * \brief NODE driver error codes.
 *******************************************************************/
typedef enum {
    // Driver errors.
    NODE_SUCCESS = 0,
    NODE_ERROR_NULL_PARAMETER,
    NODE_ERROR_NOT_SUPPORTED,
    NODE_ERROR_PROTOCOL,
    NODE_ERROR_REGISTER_ADDRESS_LIST_SIZE,
    NODE_ERROR_SCAN_PERIOD_READ,
    // Low level drivers errors.
    NODE_ERROR_BASE_ACCESS_STATUS_CODE = 0x0100,
    NODE_ERROR_BASE_ACCESS_STATUS_ADDRESS = 0x0200,
    NODE_ERROR_BASE_LPTIM = (NODE_ERROR_BASE_ACCESS_STATUS_ADDRESS + 0x0100),
    NODE_ERROR_BASE_POWER = (NODE_ERROR_BASE_LPTIM + LPTIM_ERROR_BASE_LAST),
    NODE_ERROR_BASE_ANALOG = (NODE_ERROR_BASE_POWER + POWER_ERROR_BASE_LAST),
    NODE_ERROR_BASE_UNA_DMM = (NODE_ERROR_BASE_ANALOG + ANALOG_ERROR_BASE_LAST),
    NODE_ERROR_BASE_UNA_AT = (NODE_ERROR_BASE_UNA_DMM + UNA_DMM_ERROR_BASE_LAST),
    NODE_ERROR_BASE_UNA_R4S8CR = (NODE_ERROR_BASE_UNA_AT + UNA_AT_ERROR_BASE_LAST),
    // Last base value.
    NODE_ERROR_BASE_LAST = (NODE_ERROR_BASE_UNA_R4S8CR + UNA_R4S8CR_ERROR_BASE_LAST),
} NODE_status_t;

/*** NODES global variables ***/

extern UNA_node_list_t NODE_LIST;

/*** NODE functions ***/

/*!******************************************************************
 * \fn NODE_status_t NODE_init(void)
 * \brief Init node interface.
 * \param[in]   none
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
NODE_status_t NODE_init(void);

/*!******************************************************************
 * \fn NODE_status_t NODE_de_init(void)
 * \brief Release node interface.
 * \param[in]   none
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
NODE_status_t NODE_de_init(void);

/*!******************************************************************
 * \fn NODE_status_t NODE_process(void)
 * \brief Main task of node interface.
 * \param[in]   none
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
NODE_status_t NODE_process(void);

/*!******************************************************************
 * \fn NODE_status_t NODE_write_register(UNA_node_t* node, uint8_t reg_addr, uint32_t reg_value, uint32_t reg_mask, UNA_access_status_t* write_status)
 * \brief Write node register.
 * \param[in]   node: Pointer to the node to access.
 * \param[in]   reg_addr: Address of the register to write.
 * \param[in]   reg_value: Register value to write.
 * \param[in]   reg_mask: Writing operation mask.
 * \param[out]  write_status: Pointer to the writing operation status.
 * \retval      Function execution status.
 *******************************************************************/
NODE_status_t NODE_write_register(UNA_node_t* node, uint8_t reg_addr, uint32_t reg_value, uint32_t reg_mask, UNA_access_status_t* write_status);

/*!******************************************************************
 * \fn NODE_status_t NODE_perform_measurements(UNA_node_t* node, UNA_access_status_t* write_status)
 * \brief Send the command to perform all node measurements.
 * \param[in]   node: Pointer to the node to access.
 * \param[out]  write_status: Pointer to the writing operation status.
 * \retval      Function execution status.
 *******************************************************************/
NODE_status_t NODE_perform_measurements(UNA_node_t* node, UNA_access_status_t* write_status);

/*!******************************************************************
 * \fn NODE_status_t NODE_read_register(UNA_node_address_t node_addr, uint8_t reg_addr, uint32_t* reg_value, UNA_access_status_t* read_status);
 * \brief Read node register.
 * \param[in]   node: Pointer to the node to access.
 * \param[in]   reg_addr: Address of the register to read.
 * \param[out]  reg_value: Pointer to the read register value.
 * \param[out]  read_status: Pointer to the read operation status.
 * \retval      Function execution status.
 *******************************************************************/
NODE_status_t NODE_read_register(UNA_node_t* node, uint8_t reg_addr, uint32_t* reg_value, UNA_access_status_t* read_status);

/*!******************************************************************
 * \fn NODE_status_t NODE_read_registers(UNA_node_t* node, uint8_t* reg_addr_list, uint8_t reg_addr_list_size, uint32_t* node_registers, UNA_access_status_t* read_status)
 * \brief Read node registers.
 * \param[in]   node: Pointer to the node to access.
 * \param[in]   reg_addr_list: List of the addresses of the registers to read.
 * \param[in]   reg_addr_list_size: Number of registers to read.
 * \param[out]  node_registers: Pointer to the node registers value.
 * \param[out]  read_status: Pointer to the read operation status.
 * \retval      Function execution status.
 *******************************************************************/
NODE_status_t NODE_read_registers(UNA_node_t* node, uint8_t* reg_addr_list, uint8_t reg_addr_list_size, uint32_t* node_registers, UNA_access_status_t* read_status);

/*!******************************************************************
 * \fn NODE_status_t NODE_scan(void)
 * \brief Scan all nodes connected to the RS485 bus.
 * \param[in]   none
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
NODE_status_t NODE_scan(void);

/*******************************************************************/
#define NODE_exit_error(base) { ERROR_check_exit(node_status, NODE_SUCCESS, base) }

/*******************************************************************/
#define NODE_stack_error(base) { ERROR_check_stack(node_status, NODE_SUCCESS, base) }

/*******************************************************************/
#define NODE_stack_exit_error(base, code) { ERROR_check_stack_exit(node_status, NODE_SUCCESS, base, code) }

#endif /* __NODE_H__ */
