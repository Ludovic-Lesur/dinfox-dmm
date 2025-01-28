/*
 * radio_dmm.h
 *
 *  Created on: 08 jan. 2023
 *      Author: Ludo
 */

#ifndef __RADIO_DMM_H__
#define __RADIO_DMM_H__

#include "node.h"
#include "radio.h"
#include "radio_common.h"
#include "string.h"
#include "types.h"
#include "una.h"

/*** RADIO DMM functions ***/

/*!******************************************************************
 * \fn RADIO_status_t RADIO_DMM_build_ul_node_payload(RADIO_ul_node_payload_t* node_payload)
 * \brief Build node node uplink payload.
 * \param[in]   none
 * \param[out]  node_payload: Pointer to the node uplink payload.
 * \retval      Function execution status.
 *******************************************************************/
RADIO_status_t RADIO_DMM_build_ul_node_payload(RADIO_ul_node_payload_t* node_payload);

#endif /* __RADIO_DMM_H__ */
