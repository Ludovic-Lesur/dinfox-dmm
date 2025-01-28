/*
 * hmi_gpsm.h
 *
 *  Created on: 16 apr. 2023
 *      Author: Ludo
 */

#ifndef __HMI_GPSM_H__
#define __HMI_GPSM_H__

#include "hmi_node.h"
#include "hmi_common.h"
#include "types.h"
#include "una.h"

/*** HMI GPSM structures ***/

/*!******************************************************************
 * \enum HMI_GPSM_line_index_t
 * \brief GPSM screen data lines index.
 *******************************************************************/
typedef enum {
    HMI_COMMON_LINE_INDEX(GPSM)
    HMI_GPSM_LINE_INDEX_VGPS,
    HMI_GPSM_LINE_INDEX_VANT,
    HMI_GPSM_LINE_INDEX_LAST,
} HMI_GPSM_line_index_t;

/*** HMI GPSM global variables ***/

extern const HMI_NODE_line_t HMI_GPSM_LINE[HMI_GPSM_LINE_INDEX_LAST];

#endif /* __HMI_GPSM_H__ */
