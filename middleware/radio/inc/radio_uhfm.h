/*
 * radio_uhfm.h
 *
 *  Created on: 14 feb. 2023
 *      Author: Ludo
 */

#ifndef __RADIO_UHFM_H__
#define __RADIO_UHFM_H__

#include "radio.h"
#include "types.h"
#include "una.h"

/*!******************************************************************
 * \enum UHFM_ul_message_t
 * \brief Modem uplink message parameters.
 *******************************************************************/
typedef struct {
    uint8_t* ul_payload;
    uint8_t ul_payload_size;
    uint8_t bidirectional_flag;
} UHFM_ul_message_t;

/*** UHFM functions ***/

/*!******************************************************************
 * \fn RADIO_status_t RADIO_UHFM_build_ul_node_payload(RADIO_node_t* radio_node, RADIO_ul_payload_t* node_payload)
 * \brief Build node node uplink payload.
 * \param[in]   radio_node: Node to process.
 * \param[out]  node_payload: Pointer to the node uplink payload.
 * \retval      Function execution status.
 *******************************************************************/
RADIO_status_t RADIO_UHFM_build_ul_node_payload(RADIO_node_t* radio_node, RADIO_ul_payload_t* node_payload);

/*!******************************************************************
 * \fn RADIO_status_t RADIO_UHFM_send_ul_message(UNA_node_t* uhfm_node, UHFM_ul_message_t* ul_message)
 * \brief Send Sigfox message with UHFM node.
 * \param[in]   node: Pointer to the UHFM node to use.
 * \param[in]   ul_message: Pointer to the uplink message to send.
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
RADIO_status_t RADIO_UHFM_send_ul_message(UNA_node_t* uhfm_node, UHFM_ul_message_t* ul_message);

/*!******************************************************************
 * \fn RADIO_status_t RADIO_UHFM_get_dl_payload(UNA_node_t* uhfm_node, uint8_t* dl_payload_available, uint8_t* dl_payload)
 * \brief Get last downlink payload received by UHFM node.
 * \param[in]   node: Pointer to the UHFM node to use.
 * \param[out]  dl_payload: Pointer to the downlink payload.
 * \param[out]  dl_payload_available: Pointer to the data availability flag.
 * \retval      Function execution status.
 *******************************************************************/
RADIO_status_t RADIO_UHFM_get_dl_payload(UNA_node_t* uhfm_node, uint8_t* dl_payload_available, uint8_t* dl_payload);

/*!******************************************************************
 * \fn RADIO_status_t RADIO_UHFM_get_last_bidirectional_mc(UNA_node_t* uhfm_node, uint32_t* last_bidirectional_mc)
 * \brief Get last message counter used by UHFM node.
 * \param[in]   node: Pointer to the UHFM node to use.
 * \param[out]  last_bidirectional_mc: Pointer that will contain the message counter.
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
RADIO_status_t RADIO_UHFM_get_last_bidirectional_mc(UNA_node_t* uhfm_node, uint32_t* last_bidirectional_mc);

#endif /* __RADIO_UHFM_H__ */
