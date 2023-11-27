/*
 * dmm.h
 *
 *  Created on: 08 jan. 2023
 *      Author: Ludo
 */

#ifndef __DMM_H__
#define __DMM_H__

#include "at_bus.h"
#include "common.h"
#include "dmm_reg.h"
#include "node.h"
#include "node_common.h"
#include "string.h"
#include "types.h"

/*** DMM structures ***/

/*!******************************************************************
 * \enum DMM_line_data_index_t
 * \brief DMM screen data lines index.
 *******************************************************************/
typedef enum {
	DMM_LINE_DATA_INDEX_VRS = COMMON_LINE_DATA_INDEX_LAST,
	DMM_LINE_DATA_INDEX_VHMI,
	DMM_LINE_DATA_INDEX_VUSB,
	DMM_LINE_DATA_INDEX_NODES_COUNT,
	DMM_LINE_DATA_INDEX_NODES_SCAN_PERIOD,
	DMM_LINE_DATA_INDEX_SIGFOX_UL_PERIOD,
	DMM_LINE_DATA_INDEX_SIGFOX_DL_PERIOD,
	DMM_LINE_DATA_INDEX_LAST,
} DMM_line_data_index_t;

/*** DMM global variables ***/

static const uint32_t DMM_REG_WRITE_TIMEOUT_MS[DMM_REG_ADDR_LAST] = {
	COMMON_REG_WRITE_TIMEOUT_MS_LIST
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS
};

/*** DMM functions ***/

/*!******************************************************************
 * \fn void DMM_init_registers(void)
 * \brief Init DMM registers.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		none
 *******************************************************************/
void DMM_init_registers(void);

/*!******************************************************************
 * \fn NODE_status_t DMM_write_register(NODE_access_parameters_t* write_params, uint32_t reg_value, uint32_t reg_mask, NODE_access_status_t* write_status)
 * \brief Write DMM node register.
 * \param[in]  	write_params: Pointer to the write operation parameters.
 * \param[in]	reg_value: Register value to write.
 * \param[in]	reg_mask: Writing operation mask.
 * \param[out] 	write_status: Pointer to the writing operation status.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t DMM_write_register(NODE_access_parameters_t* write_params, uint32_t reg_value, uint32_t reg_mask, NODE_access_status_t* write_status);

/*!******************************************************************
 * \fn NODE_status_t DMM_read_register(NODE_access_parameters_t* read_params, uint32_t* reg_value, NODE_access_status_t* read_status)
 * \brief Read DMM node register.
 * \param[in]  	read_params: Pointer to the read operation parameters.
 * \param[out]	reg_value: Pointer to the read register value.
 * \param[out] 	read_status: Pointer to the read operation status.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t DMM_read_register(NODE_access_parameters_t* read_params, uint32_t* reg_value, NODE_access_status_t* read_status);

/*!******************************************************************
 * \fn NODE_status_t DMM_write_line_data(NODE_line_data_write_t* line_data_write, NODE_access_status_t* write_status)
 * \brief Write corresponding node register of screen data line.
 * \param[in]  	line_data_write: Pointer to the writing operation parameters.
 * \param[out] 	write_status: Pointer to the writing operation status.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t DMM_write_line_data(NODE_line_data_write_t* line_data_write, NODE_access_status_t* write_status);

/*!******************************************************************
 * \fn NODE_status_t DMM_read_line_data(NODE_line_data_read_t* line_data_read, NODE_access_status_t* read_status)
 * \brief Read corresponding node register of screen data line.
 * \param[in]  	line_data_read: Pointer to the reading operation parameters.
 * \param[out] 	read_status: Pointer to the reading operation status.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t DMM_read_line_data(NODE_line_data_read_t* line_data_read, NODE_access_status_t* read_status);

/*!******************************************************************
 * \fn NODE_status_t DMM_build_sigfox_ul_payload(NODE_ul_payload_t* node_ul_payload)
 * \brief Build node Sigfox uplink payload.
 * \param[in]  	none
 * \param[out] 	node_ul_payload: Pointer to the Sigfox uplink payload.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t DMM_build_sigfox_ul_payload(NODE_ul_payload_t* node_ul_payload);

/*!******************************************************************
 * \fn NODE_status_t DMM_build_sigfox_action_log_ul_payload(NODE_ul_payload_t* node_ul_payload, NODE_action_t* node_action)
 * \brief Build specifc action log uplink payload.
 * \param[in]  	node_action: Pointer to the action to log.
 * \param[out] 	node_ul_payload: Pointer to the Sigfox uplink payload.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t DMM_build_sigfox_action_log_ul_payload(NODE_ul_payload_t* node_ul_payload, NODE_action_t* node_action);

#endif /* __DMM_H__ */
