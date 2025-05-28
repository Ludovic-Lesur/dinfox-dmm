/*
 * radio_mpmcm.h
 *
 *  Created on: 26 feb. 2023
 *      Author: Ludo
 */

#ifndef __RADIO_MPMCM_H__
#define __RADIO_MPMCM_H__

#include "radio.h"
#include "types.h"
#include "una.h"

/*** RADIO MPMCM structures ***/

typedef RADIO_status_t (*RADIO_MPMCM_radio_transmit_t)(UNA_node_t* node, RADIO_ul_payload_t* node_payload, uint8_t bidirectional_flag);

/*** RADIO MPMCM functions ***/

/*!******************************************************************
 * \fn RADIO_status_t RADIO_MPMCM_build_ul_node_payload(RADIO_node_t* radio_node, RADIO_ul_payload_t* node_payload)
 * \brief Build node node uplink payload.
 * \param[in]   radio_node: Node to process.
 * \param[out]  node_payload: Pointer to the node uplink payload.
 * \retval      Function execution status.
 *******************************************************************/
RADIO_status_t RADIO_MPMCM_build_ul_node_payload(RADIO_node_t* radio_node, RADIO_ul_payload_t* node_payload);

/*!******************************************************************
 * \fn RADIO_status_t RADIO_MPMCM_process(UNA_node_t* mpmcm_node, RADIO_MPMCM_radio_transmit_t radio_transmit_pfn)
 * \brief Build and send specific MPMCM node uplink payloads which has to be sent at fixed period.
 * \param[in]   mpmcm_node: Pointer to the MPMCM node address.
 * \param[in]   radio_transmit_pfn: Pointer to the radio transmission function.
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
RADIO_status_t RADIO_MPMCM_process(UNA_node_t* mpmcm_node, RADIO_MPMCM_radio_transmit_t radio_transmit_pfn);

#endif /* __RADIO_MPMCM_H__ */
