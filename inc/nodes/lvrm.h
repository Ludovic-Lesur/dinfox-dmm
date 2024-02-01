/*
 * lvrm.h
 *
 *  Created on: 21 jan. 2023
 *      Author: Ludo
 */

#ifndef __LVRM_H__
#define __LVRM_H__

#include "at_bus.h"
#include "common.h"
#include "lvrm_reg.h"
#include "mode.h"
#include "node.h"
#include "node_common.h"
#include "string.h"
#include "types.h"

/*** LVRM structures ***/

/*!******************************************************************
 * \enum LVRM_line_data_index_t
 * \brief LVRM screen data lines index.
 *******************************************************************/
typedef enum {
	LVRM_LINE_DATA_INDEX_VCOM = COMMON_LINE_DATA_INDEX_LAST,
	LVRM_LINE_DATA_INDEX_VOUT,
	LVRM_LINE_DATA_INDEX_IOUT,
	LVRM_LINE_DATA_INDEX_RLST,
	LVRM_LINE_DATA_INDEX_LAST,
} LVRM_line_data_index_t;

/*** LVRM global variables ***/

static const uint32_t LVRM_REG_WRITE_TIMEOUT_MS[LVRM_REG_ADDR_LAST] = {
	COMMON_REG_WRITE_TIMEOUT_MS_LIST
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	5000,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS
};

/*** LVRM functions ***/

/*!******************************************************************
 * \fn NODE_status_t LVRM_write_line_data(NODE_line_data_write_t* line_data_write, NODE_access_status_t* write_status)
 * \brief Write corresponding node register of screen data line.
 * \param[in]  	line_data_write: Pointer to the writing operation parameters.
 * \param[out] 	write_status: Pointer to the writing operation status.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t LVRM_write_line_data(NODE_line_data_write_t* line_data_write, NODE_access_status_t* write_status);

/*!******************************************************************
 * \fn NODE_status_t LVRM_read_line_data(NODE_line_data_read_t* line_data_read, NODE_access_status_t* read_status)
 * \brief Read corresponding node register of screen data line.
 * \param[in]  	line_data_read: Pointer to the reading operation parameters.
 * \param[out] 	read_status: Pointer to the reading operation status.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t LVRM_read_line_data(NODE_line_data_read_t* line_data_read, NODE_access_status_t* read_status);

/*!******************************************************************
 * \fn NODE_status_t LVRM_build_sigfox_ul_payload(NODE_ul_payload_t* node_ul_payload)
 * \brief Build node Sigfox uplink payload.
 * \param[in]  	none
 * \param[out] 	node_ul_payload: Pointer to the Sigfox uplink payload.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t LVRM_build_sigfox_ul_payload(NODE_ul_payload_t* node_ul_payload);

#endif /* __LVRM_H__ */
