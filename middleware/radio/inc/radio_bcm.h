/*
 * radio_bcm.h
 *
 *  Created on: 29 mar. 2025
 *      Author: Ludo
 */

#ifndef __RADIO_BCM_H__
#define __RADIO_BCM_H__

#include "radio.h"
#include "types.h"

/*** RADIO BCM functions ***/

/*!******************************************************************
 * \fn RADIO_status_t RADIO_BCM_build_ul_node_payload(RADIO_node_t* radio_node, RADIO_ul_payload_t* node_payload)
 * \brief Build BCM node uplink payload.
 * \param[in]   radio_node: Node to process.
 * \param[out]  node_payload: Pointer to the node uplink payload.
 * \retval      Function execution status.
 *******************************************************************/
RADIO_status_t RADIO_BCM_build_ul_node_payload(RADIO_node_t* radio_node, RADIO_ul_payload_t* node_payload);

#endif /* __RADIO_BCM_H__ */
