/*
 * radio_sm.h
 *
 *  Created on: 04 mar. 2023
 *      Author: Ludo
 */

#ifndef __RADIO_SM_H__
#define __RADIO_SM_H__

#include "radio.h"
#include "types.h"

/*** RADIO SM functions ***/

/*!******************************************************************
 * \fn RADIO_status_t RADIO_SM_build_ul_node_payload(RADIO_ul_node_payload_t* node_payload)
 * \brief Build node node uplink payload.
 * \param[in]  	none
 * \param[out] 	node_payload: Pointer to the node uplink payload.
 * \retval		Function execution status.
 *******************************************************************/
RADIO_status_t RADIO_SM_build_ul_node_payload(RADIO_ul_node_payload_t* node_payload);

#endif /* __RADIO_SM_H__ */
