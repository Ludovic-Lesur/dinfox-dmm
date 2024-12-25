/*
 * radio_common.h
 *
 *  Created on: 12 nov 2022
 *      Author: Ludo
 */

#ifndef __RADIO_COMMON_H__
#define __RADIO_COMMON_H__

#include "radio.h"
#include "types.h"

/*** RADIO COMMON functions ***/

/*!******************************************************************
 * \fn RADIO_status_t RADIO_COMMON_check_event_driven_payloads(RADIO_ul_node_payload_t* node_payload, uint32_t* node_registers)
 * \brief Check common flags and build associated payloads.
 * \param[in]   none
 * \param[out]	node_registers: Pointer to the node registers.
 * \param[out] 	node_payload: Pointer to the node uplink payload.
 * \retval		Function execution status.
 *******************************************************************/
RADIO_status_t RADIO_COMMON_check_event_driven_payloads(RADIO_ul_node_payload_t* node_payload, uint32_t* node_registers);

/*!******************************************************************
 * \fn RADIO_status_t RADIO_COMMON_build_ul_node_payload_action_log(RADIO_ul_node_payload_t* node_payload, RADIO_node_action_t* node_action)
 * \brief Build common action log uplink payload.
 * \param[in]  	node_action: Pointer to the action to log.
 * \param[out] 	node_payload: Pointer to the node uplink payload.
 * \retval		Function execution status.
 *******************************************************************/
RADIO_status_t RADIO_COMMON_build_ul_node_payload_action_log(RADIO_ul_node_payload_t* node_payload, RADIO_node_action_t* node_action);

#endif /* __RADIO_COMMON_H__ */
