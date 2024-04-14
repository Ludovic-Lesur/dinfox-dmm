/*
 * mpmcm.h
 *
 *  Created on: 26 feb. 2023
 *      Author: Ludo
 */

#ifndef __MPMCM_H__
#define __MPMCM_H__

#include "at_bus.h"
#include "common.h"
#include "mpmcm_reg.h"
#include "node.h"
#include "node_common.h"
#include "string.h"
#include "types.h"

/*** MPMCM structures ***/

/*!******************************************************************
 * \enum MPMCM_line_data_index_t
 * \brief MPMCM screen data lines index.
 *******************************************************************/
typedef enum {
	MPMCM_LINE_DATA_INDEX_VRMS = COMMON_LINE_DATA_INDEX_LAST,
	MPMCM_LINE_DATA_INDEX_FREQ,
	MPMCM_LINE_DATA_INDEX_CH1_PACT,
	MPMCM_LINE_DATA_INDEX_CH1_PAPP,
	MPMCM_LINE_DATA_INDEX_CH2_PACT,
	MPMCM_LINE_DATA_INDEX_CH2_PAPP,
	MPMCM_LINE_DATA_INDEX_CH3_PACT,
	MPMCM_LINE_DATA_INDEX_CH3_PAPP,
	MPMCM_LINE_DATA_INDEX_CH4_PACT,
	MPMCM_LINE_DATA_INDEX_CH4_PAPP,
	MPMCM_LINE_DATA_INDEX_TIC_PACT,
	MPMCM_LINE_DATA_INDEX_TIC_PAPP,
	MPMCM_LINE_DATA_INDEX_LAST,
} MPMCM_line_data_index_t;

/*** MPMCM global variables ***/

/*******************************************************************/
#define MPMCM_REG_WRITE_TIMEOUT_CHx \
	/* Active power */ \
	AT_BUS_DEFAULT_TIMEOUT_MS, \
	AT_BUS_DEFAULT_TIMEOUT_MS, \
	/* RMS voltage */ \
	AT_BUS_DEFAULT_TIMEOUT_MS, \
	AT_BUS_DEFAULT_TIMEOUT_MS, \
	/* RMS current */ \
	AT_BUS_DEFAULT_TIMEOUT_MS, \
	AT_BUS_DEFAULT_TIMEOUT_MS, \
	/* Apparent power */ \
	AT_BUS_DEFAULT_TIMEOUT_MS, \
	AT_BUS_DEFAULT_TIMEOUT_MS, \
	/* Power factor */ \
	AT_BUS_DEFAULT_TIMEOUT_MS, \
	AT_BUS_DEFAULT_TIMEOUT_MS, \
	/* Energy */ \
	AT_BUS_DEFAULT_TIMEOUT_MS, \

static const uint32_t MPMCM_REG_WRITE_TIMEOUT_MS[MPMCM_REG_ADDR_LAST] = {
	COMMON_REG_WRITE_TIMEOUT_MS_LIST
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	5000,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	MPMCM_REG_WRITE_TIMEOUT_CHx
	MPMCM_REG_WRITE_TIMEOUT_CHx
	MPMCM_REG_WRITE_TIMEOUT_CHx
	MPMCM_REG_WRITE_TIMEOUT_CHx
	MPMCM_REG_WRITE_TIMEOUT_CHx
};

/*** MPMCM functions ***/

/*!******************************************************************
 * \fn NODE_status_t MPMCM_write_line_data(NODE_line_data_write_t* line_data_write, NODE_access_status_t* write_status)
 * \brief Write corresponding node register of screen data line.
 * \param[in]  	line_data_write: Pointer to the writing operation parameters.
 * \param[out] 	write_status: Pointer to the writing operation status.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t MPMCM_write_line_data(NODE_line_data_write_t* line_data_write, NODE_access_status_t* write_status);

/*!******************************************************************
 * \fn NODE_status_t MPMCM_read_line_data(NODE_line_data_read_t* line_data_read, NODE_access_status_t* read_status)
 * \brief Read corresponding node register of screen data line.
 * \param[in]  	line_data_read: Pointer to the reading operation parameters.
 * \param[out] 	read_status: Pointer to the reading operation status.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t MPMCM_read_line_data(NODE_line_data_read_t* line_data_read, NODE_access_status_t* read_status);

/*!******************************************************************
 * \fn NODE_status_t MPMCM_build_sigfox_ul_payload(NODE_ul_payload_t* node_ul_payload)
 * \brief Build node Sigfox uplink payload.
 * \param[in]  	none
 * \param[out] 	node_ul_payload: Pointer to the Sigfox uplink payload.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t MPMCM_build_sigfox_ul_payload(NODE_ul_payload_t* node_ul_payload);

/*!******************************************************************
 * \fn NODE_status_t MPMCM_radio_process(NODE_address_t mpmcm_node_addr, NODE_address_t uhfm_node_addr)
 * \brief Build and send specific MPMCM Sigfox uplink payloads which has to be sent at fixed period.
 * \param[in]  	mpmcm_node_addr: MPMCM node address.
 * \param[in]	uhfm_node_addr: Radio node address.
 * \param[out]	none
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t MPMCM_radio_process(NODE_address_t mpmcm_node_addr, NODE_address_t uhfm_node_addr);

#endif /* __MPMCM_H__ */
