/*
 * hmi_bpsm.h
 *
 *  Created on: 26 feb. 2023
 *      Author: Ludo
 */

#ifndef __HMI_BPSM_H__
#define __HMI_BPSM_H__

#include "hmi_node.h"
#include "hmi_common.h"
#include "types.h"
#include "una.h"

/*** HMI BPSM structures ***/

/*!******************************************************************
 * \enum HMI_BPSM_line_index_t
 * \brief BPSM screen data lines index.
 *******************************************************************/
typedef enum {
    HMI_COMMON_LINE_INDEX(BPSM)
    HMI_BPSM_LINE_INDEX_VSRC,
    HMI_BPSM_LINE_INDEX_VSTR,
    HMI_BPSM_LINE_INDEX_VBKP,
    HMI_BPSM_LINE_INDEX_CHEN,
    HMI_BPSM_LINE_INDEX_CHST,
    HMI_BPSM_LINE_INDEX_BKEN,
    HMI_BPSM_LINE_INDEX_LAST,
} HMI_BPSM_line_index_t;

/*** HMI BPSM global variables ***/

extern const HMI_NODE_line_t HMI_BPSM_LINE[HMI_BPSM_LINE_INDEX_LAST];

#endif /* __HMI_BPSM_H__ */
