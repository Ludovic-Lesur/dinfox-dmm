/*
 * radio_gpsm.h
 *
 *  Created on: 16 apr. 2023
 *      Author: Ludo
 */

#ifndef __RADIO_GPSM_H__
#define __RADIO_GPSM_H__

#include "radio.h"
#include "types.h"

/*** RADIO GPSM functions ***/

/*!******************************************************************
 * \fn RADIO_status_t RADIO_GPSM_build_ul_node_payload(RADIO_node_t* radio_node, RADIO_ul_payload_t* node_payload)
 * \brief Build node node uplink payload.
 * \param[in]   radio_node: Node to process.
 * \param[out]  node_payload: Pointer to the node uplink payload.
 * \retval      Function execution status.
 *******************************************************************/
RADIO_status_t RADIO_GPSM_build_ul_node_payload(RADIO_node_t* radio_node, RADIO_ul_payload_t* node_payload);

#endif /* __RADIO_GPSM_H__ */
