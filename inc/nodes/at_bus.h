/*
 * at_bus.h
 *
 *  Created on: 18 feb. 2023
 *      Author: Ludo
 */

#ifndef __AT_BUS_H__
#define __AT_BUS_H__

#include "node.h"
#include "node_common.h"
#include "types.h"

/*** AT BUS macros ***/

#define AT_BUS_DEFAULT_TIMEOUT_MS	200

/*** AT BUS functions ***/

/*!******************************************************************
 * \fn NODE_status_t AT_BUS_init(void)
 * \brief Init AT BUS interface.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		none
 *******************************************************************/
void AT_BUS_init(void);

/*!******************************************************************
 * \fn NODE_status_t AT_BUS_send_command(NODE_command_parameters_t* command_params)
 * \brief Send a command over AT BUS interface.
 * \param[in]  	command_params: Pointer to the command parameters.
 * \param[out] 	none
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t AT_BUS_send_command(NODE_command_parameters_t* command_params);

/*!******************************************************************
 * \fn NODE_status_t AT_BUS_write_register(NODE_access_parameters_t* write_params, uint32_t reg_value, uint32_t reg_mask, NODE_access_status_t* write_status)
 * \brief Write node register through AT BUS interface.
 * \param[in]  	write_params: Pointer to the write operation parameters.
 * \param[in]	reg_value: Register value to write.
 * \param[in]	reg_mask: Writing operation mask.
 * \param[out] 	write_status: Pointer to the writing operation status.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t AT_BUS_write_register(NODE_access_parameters_t* write_params, uint32_t reg_value, uint32_t reg_mask, NODE_access_status_t* write_status);

/*!******************************************************************
 * \fn NODE_status_t AT_BUS_read_register(NODE_access_parameters_t* read_params, uint32_t* reg_value, NODE_access_status_t* read_status)
 * \brief Read node register through AT BUS interface.
 * \param[in]  	read_params: Pointer to the read operation parameters.
 * \param[out]	reg_value: Pointer to the read register value.
 * \param[out] 	read_status: Pointer to the read operation status.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t AT_BUS_read_register(NODE_access_parameters_t* read_params, uint32_t* reg_value, NODE_access_status_t* read_status);

/*!******************************************************************
 * \fn NODE_status_t AT_BUS_scan(NODE_t* nodes_list, uint8_t nodes_list_size, uint8_t* nodes_count)
 * \brief Scan all DINFox nodes connected to the RS485 bus.
 * \param[in]  	nodes_list_size: Maximum size of the node list.
 * \param[out]	nodes_list: Pointer to the list where to store the nodes.
 * \param[out] 	nodes_count: Pointer to the number of nodes detected.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t AT_BUS_scan(NODE_t* nodes_list, uint8_t nodes_list_size, uint8_t* nodes_count);

#endif /* __AT_BUS_H__ */
