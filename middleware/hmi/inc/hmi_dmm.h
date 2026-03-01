/*
 * hmi_dmm.h
 *
 *  Created on: 08 jan. 2023
 *      Author: Ludo
 */

#ifndef __HMI_DMM_H__
#define __HMI_DMM_H__

#include "hmi_node.h"
#include "hmi_common.h"
#include "types.h"
#include "una.h"

/*** HMI DMM structures ***/

/*!******************************************************************
 * \enum HMI_DMM_line_index_t
 * \brief DMM screen data lines index.
 *******************************************************************/
typedef enum {
    HMI_COMMON_LINE_INDEX(DMM)
    HMI_DMM_LINE_INDEX_RS485_BUS_VOLTAGE,
    HMI_DMM_LINE_INDEX_HMI_VOLTAGE,
    HMI_DMM_LINE_INDEX_USB_VOLTAGE,
    HMI_DMM_LINE_INDEX_NODE_COUNT,
    HMI_DMM_LINE_INDEX_NODE_SCAN_PERIOD,
    HMI_DMM_LINE_INDEX_SIGFOX_UL_PERIOD,
    HMI_DMM_LINE_INDEX_SIGFOX_DL_PERIOD,
    HMI_DMM_LINE_INDEX_LAST,
} HMI_DMM_line_index_t;

/*** HMI DMM global variables ***/

extern const HMI_NODE_line_t HMI_DMM_LINE[HMI_DMM_LINE_INDEX_LAST];

#endif /* __HMI_DMM_H__ */
