/*
 * sm.h
 *
 *  Created on: 04 mar. 2023
 *      Author: Ludo
 */

#ifndef __SM_H__
#define __SM_H__

#include "at_bus.h"
#include "common.h"
#include "node.h"
#include "sm_reg.h"
#include "string.h"
#include "types.h"

/*** SM structures ***/

/*!******************************************************************
 * \enum SM_line_data_index_t
 * \brief SM screen data lines index.
 *******************************************************************/
typedef enum {
	SM_LINE_DATA_INDEX_AIN0 = COMMON_LINE_DATA_INDEX_LAST,
	SM_LINE_DATA_INDEX_AIN1,
	SM_LINE_DATA_INDEX_AIN2,
	SM_LINE_DATA_INDEX_AIN3,
	SM_LINE_DATA_INDEX_DIO0,
	SM_LINE_DATA_INDEX_DIO1,
	SM_LINE_DATA_INDEX_DIO2,
	SM_LINE_DATA_INDEX_DIO3,
	SM_LINE_DATA_INDEX_TAMB,
	SM_LINE_DATA_INDEX_HAMB,
	SM_LINE_DATA_INDEX_LAST,
} SM_line_data_index_t;

/*** SM global variables ***/

static const uint32_t SM_REG_WRITE_TIMEOUT_MS[SM_REG_ADDR_LAST] = {
	COMMON_REG_WRITE_TIMEOUT_MS_LIST
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS
};

/*** SM functions ***/

/*!******************************************************************
 * \fn NODE_status_t SM_write_line_data(NODE_line_data_write_t* line_data_write, NODE_access_status_t* write_status)
 * \brief Write corresponding node register of screen data line.
 * \param[in]  	line_data_write: Pointer to the writing operation parameters.
 * \param[out] 	write_status: Pointer to the writing operation status.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t SM_write_line_data(NODE_line_data_write_t* line_data_write, NODE_access_status_t* write_status);

/*!******************************************************************
 * \fn NODE_status_t SM_read_line_data(NODE_line_data_read_t* line_data_read, NODE_access_status_t* read_status)
 * \brief Read corresponding node register of screen data line.
 * \param[in]  	line_data_read: Pointer to the reading operation parameters.
 * \param[out] 	read_status: Pointer to the reading operation status.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t SM_read_line_data(NODE_line_data_read_t* line_data_read, NODE_access_status_t* read_status);

/*!******************************************************************
 * \fn NODE_status_t SM_build_sigfox_ul_payload(NODE_ul_payload_t* node_ul_payload)
 * \brief Build node Sigfox uplink payload.
 * \param[in]  	none
 * \param[out] 	node_ul_payload: Pointer to the Sigfox uplink payload.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t SM_build_sigfox_ul_payload(NODE_ul_payload_t* node_ul_payload);

#endif /* __SM_H__ */
