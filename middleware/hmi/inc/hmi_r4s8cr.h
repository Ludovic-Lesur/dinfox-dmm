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
    HMI_R4S8CR_LINE_INDEX_R1ST = 0,
    HMI_R4S8CR_LINE_INDEX_R2ST,
    HMI_R4S8CR_LINE_INDEX_R3ST,
    HMI_R4S8CR_LINE_INDEX_R4ST,
    HMI_R4S8CR_LINE_INDEX_R5ST,
    HMI_R4S8CR_LINE_INDEX_R6ST,
    HMI_R4S8CR_LINE_INDEX_R7ST,
    HMI_R4S8CR_LINE_INDEX_R8ST,
    HMI_R4S8CR_LINE_INDEX_LAST,
} HMI_R4S8CR_line_index_t;

/*** HMI R4S8CR global variables ***/

extern const HMI_NODE_line_t HMI_R4S8CR_LINE[HMI_R4S8CR_LINE_INDEX_LAST];

#endif /* __HMI_R4S8CR_H__ */
