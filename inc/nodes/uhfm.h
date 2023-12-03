/*
 * uhfm.h
 *
 *  Created on: 14 feb. 2023
 *      Author: Ludo
 */

#ifndef __UHFM_H__
#define __UHFM_H__

#include "at_bus.h"
#include "common.h"
#include "node.h"
#include "node_common.h"
#include "string.h"
#include "types.h"
#include "uhfm_reg.h"

/*** UHFM structures ***/

/*!******************************************************************
 * \enum UHFM_line_data_index_t
 * \brief UHFM screen data lines index.
 *******************************************************************/
typedef enum {
	UHFM_LINE_DATA_INDEX_SIGFOX_EP_ID = COMMON_LINE_DATA_INDEX_LAST,
	UHFM_LINE_DATA_INDEX_VRF_TX,
	UHFM_LINE_DATA_INDEX_VRF_RX,
	UHFM_LINE_DATA_INDEX_LAST,
} UHFM_line_data_index_t;

/*!******************************************************************
 * \enum UHFM_sigfox_message_t
 * \brief Sigfox message parameters.
 *******************************************************************/
typedef struct {
	uint8_t* ul_payload;
	uint8_t ul_payload_size;
	uint8_t bidirectional_flag;
} UHFM_sigfox_message_t;

/*** UHFM global variables ***/

static const uint32_t UHFM_REG_WRITE_TIMEOUT_MS[UHFM_REG_ADDR_LAST] = {
	COMMON_REG_WRITE_TIMEOUT_MS_LIST
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	5000,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS
};

/*** UHFM functions ***/

/*!******************************************************************
 * \fn NODE_status_t UHFM_write_line_data(NODE_line_data_write_t* line_data_write, NODE_access_status_t* write_status)
 * \brief Write corresponding node register of screen data line.
 * \param[in]  	line_data_write: Pointer to the writing operation parameters.
 * \param[out] 	write_status: Pointer to the writing operation status.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t UHFM_write_line_data(NODE_line_data_write_t* line_data_write, NODE_access_status_t* write_status);

/*!******************************************************************
 * \fn NODE_status_t UHFM_read_line_data(NODE_line_data_read_t* line_data_read, NODE_access_status_t* read_status)
 * \brief Read corresponding node register of screen data line.
 * \param[in]  	line_data_read: Pointer to the reading operation parameters.
 * \param[out] 	read_status: Pointer to the reading operation status.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t UHFM_read_line_data(NODE_line_data_read_t* line_data_read, NODE_access_status_t* read_status);

/*!******************************************************************
 * \fn NODE_status_t UHFM_build_sigfox_ul_payload(NODE_ul_payload_t* node_ul_payload)
 * \brief Build node Sigfox uplink payload.
 * \param[in]  	none
 * \param[out] 	node_ul_payload: Pointer to the Sigfox uplink payload.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t UHFM_build_sigfox_ul_payload(NODE_ul_payload_t* node_ul_payload);

/*!******************************************************************
 * \fn NODE_status_t UHFM_send_sigfox_message(NODE_address_t node_addr, UHFM_sigfox_message_t* sigfox_message, NODE_access_status_t* send_status)
 * \brief Send Sigfox message with UHFM node.
 * \param[in]  	node_addr: Address of the UHFM node to use.
 * \param[in]	sigfox_message: Pointer to the Sigfox message to send.
 * \param[out] 	send_status: Pointer to the sending operation status.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t UHFM_send_sigfox_message(NODE_address_t node_addr, UHFM_sigfox_message_t* sigfox_message, NODE_access_status_t* send_status);

/*!******************************************************************
 * \fn NODE_status_t UHFM_get_dl_payload(NODE_address_t node_addr, uint8_t* dl_payload, NODE_access_status_t* read_status)
 * \brief Get last downlink payload received by UHFM node.
 * \param[in]  	node_addr: Address of the UHFM node to use.
 * \param[out]	dl_payload: Pointer to the Sigfox downlink payload.
 * \param[out] 	send_status: Pointer to the read operation status.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t UHFM_get_dl_payload(NODE_address_t node_addr, uint8_t* dl_payload, NODE_access_status_t* read_status);

/*!******************************************************************
 * \fn NODE_status_t UHFM_get_last_bidirectional_mc(NODE_address_t node_addr, uint32_t* last_message_counter, NODE_access_status_t* read_status)
 * \brief Get last message counter used by UHFM node.
 * \param[in]  	node_addr: Address of the UHFM node to use.
 * \param[out]	last_message_counter: Pointer that will contain the message counter.
 * \param[out] 	send_status: Pointer to the read operation status.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t UHFM_get_last_bidirectional_mc(NODE_address_t node_addr, uint32_t* last_message_counter, NODE_access_status_t* read_status);

#endif /* __UHFM_H__ */
