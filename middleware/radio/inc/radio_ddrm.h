/*
 * radio_ddrm.h
 *
 *  Created on: 26 feb. 2023
 *      Author: Ludo
 */

#ifndef __RADIO_DDRM_H__
#define __RADIO_DDRM_H__

#include "radio.h"
#include "types.h"

/*** RADIO DDRM functions ***/

/*!******************************************************************
 * \fn RADIO_status_t DDRM_build_ul_node_payload(RADIO_ul_node_payload_t* node_payload)
 * \brief Build node node uplink payload.
 * \param[in]   none
 * \param[out]  node_payload: Pointer to the node uplink payload.
 * \retval      Function execution status.
 *******************************************************************/
RADIO_status_t RADIO_DDRM_build_ul_node_payload(RADIO_ul_node_payload_t* node_payload);

#endif /* __RADIO_DDRM_H__ */
