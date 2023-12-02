/*
 * ddrm.h
 *
 *  Created on: 26 feb. 2023
 *      Author: Ludo
 */

#ifndef __DDRM_H__
#define __DDRM_H__

#include "at_bus.h"
#include "common.h"
#include "ddrm_reg.h"
#include "node.h"
#include "node_common.h"
#include "string.h"
#include "types.h"

/*** DDRM structures ***/

/*!******************************************************************
 * \enum DDRM_line_data_index_t
 * \brief DDRM screen data lines index.
 *******************************************************************/
typedef enum {
	DDRM_LINE_DATA_INDEX_VIN = COMMON_LINE_DATA_INDEX_LAST,
	DDRM_LINE_DATA_INDEX_VOUT,
	DDRM_LINE_DATA_INDEX_IOUT,
	DDRM_LINE_DATA_INDEX_DDEN,
	DDRM_LINE_DATA_INDEX_LAST,
} DDRM_line_data_index_t;

/*** DDRM global variables ***/

static const uint32_t DDRM_REG_WRITE_TIMEOUT_MS[DDRM_REG_ADDR_LAST] = {
	COMMON_REG_WRITE_TIMEOUT_MS_LIST
	AT_BUS_DEFAULT_TIMEOUT_MS,
	1000,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS
};

/*** DDRM functions ***/

/*!******************************************************************
 * \fn NODE_status_t DDRM_write_line_data(NODE_line_data_write_t* line_data_write, NODE_access_status_t* write_status)
 * \brief Write corresponding node register of screen data line.
 * \param[in]  	line_data_write: Pointer to the writing operation parameters.
 * \param[out] 	write_status: Pointer to the writing operation status.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t DDRM_write_line_data(NODE_line_data_write_t* line_data_write, NODE_access_status_t* write_status);

/*!******************************************************************
 * \fn NODE_status_t DDRM_read_line_data(NODE_line_data_read_t* line_data_read, NODE_access_status_t* read_status)
 * \brief Read corresponding node register of screen data line.
 * \param[in]  	line_data_read: Pointer to the reading operation parameters.
 * \param[out] 	read_status: Pointer to the reading operation status.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t DDRM_read_line_data(NODE_line_data_read_t* line_data_read, NODE_access_status_t* read_status);

/*!******************************************************************
 * \fn NODE_status_t DDRM_build_sigfox_ul_payload(NODE_ul_payload_t* node_ul_payload)
 * \brief Build node Sigfox uplink payload.
 * \param[in]  	none
 * \param[out] 	node_ul_payload: Pointer to the Sigfox uplink payload.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t DDRM_build_sigfox_ul_payload(NODE_ul_payload_t* node_ul_payload);

#endif /* __DDRM_H__ */
