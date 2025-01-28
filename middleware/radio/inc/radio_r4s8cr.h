/*
 * radio_r4s8cr.h
 *
 *  Created on: 05 feb. 2023
 *      Author: Ludo
 */

#ifndef __RADIO_R4S8CR_H__
#define __RADIO_R4S8CR_H__

#include "radio.h"
#include "types.h"

/*** RADIO R4S8CR functions ***/

/*!******************************************************************
 * \fn RADIO_status_t RADIO_R4S8CR_build_ul_node_payload(RADIO_ul_node_payload_t* node_payload)
 * \brief Build node node uplink payload.
 * \param[in]   none
 * \param[out]  node_payload: Pointer to the node uplink payload.
 * \retval      Function execution status.
 *******************************************************************/
RADIO_status_t RADIO_R4S8CR_build_ul_node_payload(RADIO_ul_node_payload_t* node_payload);

#endif /* __RADIO_R4S8CR_H__ */
