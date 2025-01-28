/*
 * hmi_lvrm.h
 *
 *  Created on: 21 jan. 2023
 *      Author: Ludo
 */

#ifndef __HMI_LVRM_H__
#define __HMI_LVRM_H__

#include "hmi_node.h"
#include "hmi_common.h"
#include "types.h"
#include "una.h"

/*** HMI LVRM structures ***/

/*!******************************************************************
 * \enum HMI_LVRM_line_index_t
 * \brief LVRM screen data lines index.
 *******************************************************************/
typedef enum {
    HMI_COMMON_LINE_INDEX(LVRM)
    HMI_LVRM_LINE_INDEX_VCOM,
    HMI_LVRM_LINE_INDEX_VOUT,
    HMI_LVRM_LINE_INDEX_IOUT,
    HMI_LVRM_LINE_INDEX_RLST,
    HMI_LVRM_LINE_INDEX_LAST,
} HMI_LVRM_line_index_t;

/*** HMI LVRM global variables ***/

extern const HMI_NODE_line_t HMI_LVRM_LINE[HMI_LVRM_LINE_INDEX_LAST];

#endif /* __HMI_LVRM_H__ */
