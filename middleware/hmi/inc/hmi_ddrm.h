/*
 * ddrm.h
 *
 *  Created on: 26 feb. 2023
 *      Author: Ludo
 */

#ifndef __HMI_DDRM_H__
#define __HMI_DDRM_H__

#include "hmi_node.h"
#include "hmi_common.h"
#include "types.h"
#include "una.h"

/*** HMI DDRM structures ***/

/*!******************************************************************
 * \enum HMI_DDRM_line_index_t
 * \brief HMI_DDRM screen data lines index.
 *******************************************************************/
typedef enum {
    HMI_COMMON_LINE_INDEX(DDRM)
	HMI_DDRM_LINE_INDEX_VIN,
	HMI_DDRM_LINE_INDEX_VOUT,
	HMI_DDRM_LINE_INDEX_IOUT,
	HMI_DDRM_LINE_INDEX_DDEN,
	HMI_DDRM_LINE_INDEX_LAST,
} HMI_DDRM_line_index_t;

/*** HMI DDRM global variables ***/

extern const HMI_NODE_line_t HMI_DDRM_LINE[HMI_DDRM_LINE_INDEX_LAST];

#endif /* __HMI_DDRM_H__ */
