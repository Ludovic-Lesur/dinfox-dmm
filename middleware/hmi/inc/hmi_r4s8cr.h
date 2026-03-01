/*
 * hmi_r4s8cr.h
 *
 *  Created on: 02 feb. 2023
 *      Author: Ludo
 */

#ifndef __HMI_R4S8CR_H__
#define __HMI_R4S8CR_H__

#include "hmi_node.h"
#include "types.h"
#include "una.h"

/*** HMI R4S8CR structures ***/

/*!******************************************************************
 * \enum HMI_R4S8CR_line_index_t
 * \brief R4S8CR screen data lines index.
 *******************************************************************/
typedef enum {
    HMI_R4S8CR_LINE_INDEX_RELAY1_STATUS = 0,
    HMI_R4S8CR_LINE_INDEX_RELAY2_STATUS,
    HMI_R4S8CR_LINE_INDEX_RELAY3_STATUS,
    HMI_R4S8CR_LINE_INDEX_RELAY4_STATUS,
    HMI_R4S8CR_LINE_INDEX_RELAY5_STATUS,
    HMI_R4S8CR_LINE_INDEX_RELAY6_STATUS,
    HMI_R4S8CR_LINE_INDEX_RELAY7_STATUS,
    HMI_R4S8CR_LINE_INDEX_RELAY8_STATUS,
    HMI_R4S8CR_LINE_INDEX_LAST,
} HMI_R4S8CR_line_index_t;

/*** HMI R4S8CR global variables ***/

extern const HMI_NODE_line_t HMI_R4S8CR_LINE[HMI_R4S8CR_LINE_INDEX_LAST];

#endif /* __HMI_R4S8CR_H__ */
