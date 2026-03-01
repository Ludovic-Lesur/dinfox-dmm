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
    HMI_BCM_LINE_INDEX_SOURCE_VOLTAGE,
    HMI_BCM_LINE_INDEX_STORAGE_VOLTAGE,
    HMI_BCM_LINE_INDEX_CHARGE_CURRENT,
    HMI_BCM_LINE_INDEX_BACKUP_VOLTAGE,
    HMI_BCM_LINE_INDEX_CHARGE_CONTROL_STATE,
    HMI_BCM_LINE_INDEX_CHARGE_STATUS0,
    HMI_BCM_LINE_INDEX_CHARGE_STATUS1,
    HMI_BCM_LINE_INDEX_BACKUP_CONTROL_STATE,
    HMI_BCM_LINE_INDEX_LAST,
} HMI_BCM_line_index_t;

/*** HMI BCM global variables ***/

extern const HMI_NODE_line_t HMI_BCM_LINE[HMI_BCM_LINE_INDEX_LAST];

#endif /* __HMI_BCM_H__ */
