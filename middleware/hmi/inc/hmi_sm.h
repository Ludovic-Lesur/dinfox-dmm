/*
 * hmi_sm.h
 *
 *  Created on: 04 mar. 2023
 *      Author: Ludo
 */

#ifndef __HMI_SM_H__
#define __HMI_SM_H__

#include "hmi_node.h"
#include "hmi_common.h"
#include "types.h"
#include "una.h"

/*** HMI SM structures ***/

/*!******************************************************************
 * \enum HMI_SM_line_index_t
 * \brief SM screen data lines index.
 *******************************************************************/
typedef enum {
    HMI_COMMON_LINE_INDEX(SM)
    HMI_SM_LINE_INDEX_AIN0,
    HMI_SM_LINE_INDEX_AIN1,
    HMI_SM_LINE_INDEX_AIN2,
    HMI_SM_LINE_INDEX_AIN3,
    HMI_SM_LINE_INDEX_DIO0,
    HMI_SM_LINE_INDEX_DIO1,
    HMI_SM_LINE_INDEX_DIO2,
    HMI_SM_LINE_INDEX_DIO3,
    HMI_SM_LINE_INDEX_TAMB,
    HMI_SM_LINE_INDEX_HAMB,
    HMI_SM_LINE_INDEX_LAST,
} HMI_SM_line_index_t;

/*** HMI SM global variables ***/

extern const HMI_NODE_line_t HMI_SM_LINE[HMI_SM_LINE_INDEX_LAST];

#endif /* __HMI_SM_H__ */
