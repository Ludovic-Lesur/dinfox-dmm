/*
 * hmi_bcm.h
 *
 *  Created on: 29 mar. 2025
 *      Author: Ludo
 */

#ifndef __HMI_BCM_H__
#define __HMI_BCM_H__

#include "hmi_node.h"
#include "hmi_common.h"
#include "types.h"
#include "una.h"

/*** HMI BCM structures ***/

/*!******************************************************************
 * \enum HMI_BCM_line_index_t
 * \brief BCM screen data lines index.
 *******************************************************************/
typedef enum {
    HMI_COMMON_LINE_INDEX(BCM)
    HMI_BCM_LINE_INDEX_VSRC,
    HMI_BCM_LINE_INDEX_VSTR,
    HMI_BCM_LINE_INDEX_ISTR,
    HMI_BCM_LINE_INDEX_VBKP,
    HMI_BCM_LINE_INDEX_CHEN,
    HMI_BCM_LINE_INDEX_CHST0,
    HMI_BCM_LINE_INDEX_CHST1,
    HMI_BCM_LINE_INDEX_BKEN,
    HMI_BCM_LINE_INDEX_LAST,
} HMI_BCM_line_index_t;

/*** HMI BCM global variables ***/

extern const HMI_NODE_line_t HMI_BCM_LINE[HMI_BCM_LINE_INDEX_LAST];

#endif /* __HMI_BCM_H__ */
