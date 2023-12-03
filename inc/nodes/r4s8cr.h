/*
 * r4s8cr.h
 *
 *  Created on: 2 feb. 2023
 *      Author: Ludo
 */

#ifndef __R4S8CR_H__
#define __R4S8CR_H__

#include "node.h"
#include "node_common.h"
#include "r4s8cr_reg.h"

/*** R4S8CR macros ***/

#define R4S8CR_READ_TIMEOUT_MS		200
#define R4S8CR_WRITE_TIMEOUT_MS		5000

/*** R4S8CR structures ***/

/*!******************************************************************
 * \enum R4S8CR_line_data_index_t
 * \brief R4S8CR screen data lines index.
 *******************************************************************/
typedef enum {
	R4S8CR_LINE_DATA_INDEX_R1ST = 0,
	R4S8CR_LINE_DATA_INDEX_R2ST,
	R4S8CR_LINE_DATA_INDEX_R3ST,
	R4S8CR_LINE_DATA_INDEX_R4ST,
	R4S8CR_LINE_DATA_INDEX_R5ST,
	R4S8CR_LINE_DATA_INDEX_R6ST,
	R4S8CR_LINE_DATA_INDEX_R7ST,
	R4S8CR_LINE_DATA_INDEX_R8ST,
	R4S8CR_LINE_DATA_INDEX_LAST,
} R4S8CR_line_data_index_t;

static const uint32_t R4S8CR_REG_WRITE_TIMEOUT_MS[R4S8CR_REG_ADDR_LAST] = {
	R4S8CR_READ_TIMEOUT_MS,
	R4S8CR_WRITE_TIMEOUT_MS
};

/*** R4S8CR functions ***/

/*!******************************************************************
 * \fn void R4S8CR_init_registers(void)
 * \brief Init R4S8CR registers.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		none
 *******************************************************************/
void R4S8CR_init_registers(void);

/*!******************************************************************
 * \fn NODE_status_t R4S8CR_write_register(NODE_access_parameters_t* write_params, uint32_t reg_value, uint32_t reg_mask, NODE_access_status_t* write_status)
 * \brief Write R4S8CR node register.
 * \param[in]  	write_params: Pointer to the write operation parameters.
 * \param[in]	reg_value: Register value to write.
 * \param[in]	reg_mask: Writing operation mask.
 * \param[in]	access_error_stack: Stack node access status error is non-zero.
 * \param[out] 	write_status: Pointer to the writing operation status.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t R4S8CR_write_register(NODE_access_parameters_t* write_params, uint32_t reg_value, uint32_t reg_mask, NODE_access_status_t* write_statu, uint8_t access_error_stacks);

/*!******************************************************************
 * \fn NODE_status_t R4S8CR_read_register(NODE_access_parameters_t* read_params, uint32_t* reg_value, NODE_access_status_t* read_status)
 * \brief Read R4S8CR node register.
 * \param[in]  	read_params: Pointer to the read operation parameters.
 * \param[in]	access_error_stack: Stack node access status error is non-zero.
 * \param[out]	reg_value: Pointer to the read register value.
 * \param[out] 	read_status: Pointer to the read operation status.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t R4S8CR_read_register(NODE_access_parameters_t* read_params, uint32_t* reg_value, NODE_access_status_t* read_status, uint8_t access_error_stack);

/*!******************************************************************
 * \fn NODE_status_t R4S8CR_scan(NODE_t* nodes_list, uint8_t nodes_list_size, uint8_t* nodes_count)
 * \brief Scan all R4S8CR nodes connected to the RS485 bus.
 * \param[in]  	nodes_list_size: Maximum size of the node list.
 * \param[out]	nodes_list: Pointer to the list where to store the nodes.
 * \param[out] 	nodes_count: Pointer to the number of nodes detected.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t R4S8CR_scan(NODE_t* nodes_list, uint8_t nodes_list_size, uint8_t* nodes_count);

/*!******************************************************************
 * \fn NODE_status_t R4S8CR_write_line_data(NODE_line_data_write_t* line_data_write, NODE_access_status_t* write_status)
 * \brief Write corresponding node register of screen data line.
 * \param[in]  	line_data_write: Pointer to the writing operation parameters.
 * \param[out] 	write_status: Pointer to the writing operation status.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t R4S8CR_write_line_data(NODE_line_data_write_t* line_data_write, NODE_access_status_t* write_status);

/*!******************************************************************
 * \fn NODE_status_t R4S8CR_read_line_data(NODE_line_data_read_t* line_data_read, NODE_access_status_t* read_status)
 * \brief Read corresponding node register of screen data line.
 * \param[in]  	line_data_read: Pointer to the reading operation parameters.
 * \param[out] 	read_status: Pointer to the reading operation status.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t R4S8CR_read_line_data(NODE_line_data_read_t* line_data_read, NODE_access_status_t* read_status);

/*!******************************************************************
 * \fn NODE_status_t R4S8CR_build_sigfox_ul_payload(NODE_ul_payload_t* node_ul_payload)
 * \brief Build node Sigfox uplink payload.
 * \param[in]  	none
 * \param[out] 	node_ul_payload: Pointer to the Sigfox uplink payload.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t R4S8CR_build_sigfox_ul_payload(NODE_ul_payload_t* node_ul_payload);

#endif /* __R4S8CR_H__ */
