/*
 * xm.h
 *
 *  Created on: 14 jun. 2023
 *      Author: Ludo
 */

#ifndef __XM_H__
#define __XM_H__

#include "node.h"
#include "node_common.h"

/*** XM structures ***/

/*!******************************************************************
 * \enum XM_registers_list_t
 * \brief Node registers list type.
 *******************************************************************/
typedef struct {
	uint8_t* addr_list;
	uint8_t size;
} XM_registers_list_t;

/*!******************************************************************
 * \enum XM_node_registers_t
 * \brief Node registers value and error value.
 *******************************************************************/
typedef struct {
	uint32_t* value;
	uint32_t* error;
} XM_node_registers_t;

/*** XM functions ***/

/*!******************************************************************
 * \fn NODE_status_t XM_write_register(NODE_address_t node_addr, uint8_t reg_addr, uint32_t reg_value, uint32_t reg_mask, uint32_t timeout_ms, NODE_access_status_t* write_status)
 * \brief Write node register.
 * \param[in]  	node_addr: Address of the destination node.
 * \param[in]	reg_addr: Address of the register to write.
 * \param[in]	reg_value: Register value to write.
 * \param[in]	reg_mask: Writing operation mask.
 * \param[in]	timeout_ms: Writing operation timeout in ms.
 * \param[out] 	write_status: Pointer to the writing operation status.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t XM_write_register(NODE_address_t node_addr, uint8_t reg_addr, uint32_t reg_value, uint32_t reg_mask, uint32_t timeout_ms, NODE_access_status_t* write_status);

/*!******************************************************************
 * \fn NODE_status_t XM_read_register(NODE_address_t node_addr, uint8_t reg_addr, uint32_t reg_error_value, uint32_t* reg_value, NODE_access_status_t* read_status)
 * \brief Read node register.
 * \param[in]  	node_addr: Address of the destination node.
 * \param[out]	reg_addr: Address of the register to read.
 * \param[out]	node_reg: Pointer to the node registers and error values.
 * \param[out] 	read_status: Pointer to the read operation status.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t XM_read_register(NODE_address_t node_addr, uint8_t reg_addr, XM_node_registers_t* node_reg, NODE_access_status_t* read_status);

/*!******************************************************************
 * \fn NODE_status_t XM_read_registers(NODE_address_t node_addr, XM_registers_list_t* reg_list, XM_node_registers_t* node_reg)
 * \brief Read a list of registers.
 * \param[in]  	node_addr: Address of the destination node.
 * \param[in]	reg_list: List of registers to read.
 * \param[out]	node_reg: Pointer to the node registers and error values.
 * \param[out] 	read_status: Pointer to the read operation status.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t XM_read_registers(NODE_address_t node_addr, XM_registers_list_t* reg_list, XM_node_registers_t* node_reg, NODE_access_status_t* read_status);

/*!******************************************************************
 * \fn NODE_status_t XM_write_line_data(NODE_line_data_write_t* line_data_write, NODE_line_data_t* node_line_data, uint32_t* node_write_timeout, NODE_access_status_t* write_status)
 * \brief Write corresponding node register of screen data line.
 * \param[in]  	line_data_write: Pointer to the writing operation parameters.
 * \param[in]	node_line_data: Pointer to the displayed data lines of the node.
 * \param[in]	node_write_timeout: Pointer to the write timeout values.
 * \param[out] 	write_status: Pointer to the writing operation status.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t XM_write_line_data(NODE_line_data_write_t* line_data_write, NODE_line_data_t* node_line_data, uint32_t* node_write_timeout, NODE_access_status_t* write_status);

/*!******************************************************************
 * \fn NODE_status_t XM_perform_measurements(NODE_address_t node_addr, NODE_access_status_t* write_status)
 * \brief Perform node measurements.
 * \param[in]  	node_addr: Address of the destination node.
 * \param[out] 	write_status: Pointer to the writing operation status.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t XM_perform_measurements(NODE_address_t node_addr, NODE_access_status_t* write_status);

#endif /* __XM_H__ */
