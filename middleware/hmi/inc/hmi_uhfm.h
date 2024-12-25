/*
 * hmi_uhfm.h
 *
 *  Created on: 14 feb. 2023
 *      Author: Ludo
 */

#ifndef __HMI_UHFM_H__
#define __HMI_UHFM_H__

#include "hmi_node.h"
#include "hmi_common.h"
#include "types.h"
#include "una.h"

/*** HMI UHFM structures ***/

/*!******************************************************************
 * \enum HMI_UHFM_line_index_t
 * \brief UHFM screen data lines index.
 *******************************************************************/
typedef enum {
    HMI_COMMON_LINE_INDEX(UHFM)
	HMI_UHFM_LINE_INDEX_EP_ID,
	HMI_UHFM_LINE_INDEX_VRF_TX,
	HMI_UHFM_LINE_INDEX_VRF_RX,
	HMI_UHFM_LINE_INDEX_LAST,
} HMI_UHFM_line_index_t;

/*** HMI UHFM global variables ***/

extern const HMI_NODE_line_t HMI_UHFM_LINE[HMI_UHFM_LINE_INDEX_LAST];

#endif /* __HMI_UHFM_H__ */
