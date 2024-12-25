/*
 * radio.h
 *
 *  Created on: 18 dec. 2024
 *      Author: Ludo
 */

#ifndef __RADIO_H__
#define __RADIO_H__

#include "node.h"
#include "types.h"
#include "una.h"

/*!******************************************************************
 * \enum RADIO_status_t
 * \brief Radio driver error codes.
 *******************************************************************/
typedef enum {
    // Driver errors.
    RADIO_SUCCESS = 0,
    RADIO_ERROR_NULL_PARAMETER,
    RADIO_ERROR_MASTER_NODE_NOT_FOUND,
    RADIO_ERROR_MODEM_NODE_NOT_FOUND,
    RADIO_ERROR_MODEM_UL_CONFIGURATION,
    RADIO_ERROR_MODEM_UL_PAYLOAD,
    RADIO_ERROR_MODEM_UL_TRANSMISSION,
    RADIO_ERROR_MODEM_DL_MESSAGE_STATUS,
    RADIO_ERROR_MODEM_DL_PAYLOAD,
    RADIO_ERROR_MODEM_DL_BIDIRECTIONAL_MC,
    RADIO_ERROR_MODEM_NODE_RECEIVE,
    RADIO_ERROR_MODEM_NODE_MC,
    RADIO_ERROR_NODE_INDEX,
    RADIO_ERROR_UL_NODE_PAYLOAD_TYPE,
    RADIO_ERROR_UL_NODE_PAYLOAD_SIZE_UNDERFLOW,
    RADIO_ERROR_UL_NODE_PAYLOAD_SIZE_OVERFLOW,
    RADIO_ERROR_DL_OPERATION_CODE,
    RADIO_ERROR_ACTION_LIST_OVERFLOW,
    RADIO_ERROR_ACTION_LIST_INDEX,
    RADIO_ERROR_ACTION_NODE_ADDRESS,
    RADIO_ERROR_ACTION_READ_ACCESS,
    // Low level drivers errors.
    RADIO_ERROR_BASE_NODE = 0x0100,
    // Last base value.
    RADIO_ERROR_BASE_LAST = (RADIO_ERROR_BASE_NODE + NODE_ERROR_BASE_LAST)
} RADIO_status_t;

/*!******************************************************************
 * \enum RADIO_ul_node_payload_t
 * \brief Node UL payload structure.
 *******************************************************************/
typedef struct {
    UNA_node_t* node;
    uint8_t payload_type_counter;
    uint8_t* payload;
    uint8_t payload_size;
} RADIO_ul_node_payload_t;

/*!******************************************************************
 * \enum RADIO_node_action_t
 * \brief Radio node action structure.
 *******************************************************************/
typedef struct {
    UNA_node_t* node;
    uint32_t downlink_hash;
    uint32_t timestamp_seconds;
    uint8_t reg_addr;
    uint32_t reg_value;
    uint32_t reg_mask;
    UNA_access_status_t access_status;
} RADIO_node_action_t;

/*** RADIO functions ***/

/*!******************************************************************
 * \fn RADIO_status_t RADIO_init(void)
 * \brief Init radio layer.
 * \param[in]   none
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
RADIO_status_t RADIO_init(void);

/*!******************************************************************
 * \fn RADIO_status_t RADIO_de_init(void)
 * \brief Release radio layer.
 * \param[in]   none
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
RADIO_status_t RADIO_de_init(void);

/*!******************************************************************
 * \fn RADIO_status_t RADIO_process(void)
 * \brief Process radio layer.
 * \param[in]   none
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
RADIO_status_t RADIO_process(void);

/*******************************************************************/
#define RADIO_exit_error(base) { ERROR_check_exit(radio_status, RADIO_SUCCESS, base) }

/*******************************************************************/
#define RADIO_stack_error(base) { ERROR_check_stack(radio_status, RADIO_SUCCESS, base) }

/*******************************************************************/
#define RADIO_stack_exit_error(base, code) { ERROR_check_stack_exit(radio_status, RADIO_SUCCESS, base, code) }

#endif /* __RADIO_H__ */
