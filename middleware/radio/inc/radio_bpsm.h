/*
 * radio_bpsm.h
 *
 *  Created on: 26 feb. 2023
 *      Author: Ludo
 */

#ifndef __RADIO_BPSM_H__
#define __RADIO_BPSM_H__

#include "radio.h"
#include "types.h"

/*** RADIO BPSM functions ***/

/*!******************************************************************
 * \fn RADIO_status_t RADIO_BPSM_build_ul_node_payload(RADIO_node_t* radio_node, RADIO_ul_payload_t* node_payload)
 * \brief Build BPSM node uplink payload.
 * \param[in]   radio_node: Node to process.
 * \param[out]  node_payload: Pointer to the node uplink payload.
 * \retval      Function execution status.
 *******************************************************************/
RADIO_status_t RADIO_BPSM_build_ul_node_payload(RADIO_node_t* radio_node, RADIO_ul_payload_t* node_payload);

#endif /* __RADIO_BPSM_H__ */
