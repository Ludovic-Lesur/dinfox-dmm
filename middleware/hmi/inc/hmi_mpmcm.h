/*
 * hmi_mpmcm.h
 *
 *  Created on: 26 feb. 2023
 *      Author: Ludo
 */

#ifndef __HMI_MPMCM_H__
#define __HMI_MPMCM_H__

#include "hmi_node.h"
#include "hmi_common.h"
#include "types.h"
#include "una.h"

/*** HMI MPMCM structures ***/

/*!******************************************************************
 * \enum HMI_MPMCM_line_index_t
 * \brief MPMCM screen data lines index.
 *******************************************************************/
typedef enum {
    HMI_COMMON_LINE_INDEX(MPMCM)
    HMI_MPMCM_LINE_INDEX_MAINS_VOLTAGE_RMS,
    HMI_MPMCM_LINE_INDEX_MAINS_FREQUENCY,
    HMI_MPMCM_LINE_INDEX_CH1_ACTIVE_POWER,
    HMI_MPMCM_LINE_INDEX_CH1_APPARENT_POWER,
    HMI_MPMCM_LINE_INDEX_CH2_ACTIVE_POWER,
    HMI_MPMCM_LINE_INDEX_CH2_APPARENT_POWER,
    HMI_MPMCM_LINE_INDEX_CH3_ACTIVE_POWER,
    HMI_MPMCM_LINE_INDEX_CH3_APPARENT_POWER,
    HMI_MPMCM_LINE_INDEX_CH4_ACTIVE_POWER,
    HMI_MPMCM_LINE_INDEX_CH4_APPARENT_POWER,
    HMI_MPMCM_LINE_INDEX_TIC_ACTIVE_POWER,
    HMI_MPMCM_LINE_INDEX_TIC_APPARENT_POWER,
    HMI_MPMCM_LINE_INDEX_LAST,
} HMI_MPMCM_line_index_t;

/*** HMI MPMCM global variables ***/

extern const HMI_NODE_line_t HMI_MPMCM_LINE[HMI_MPMCM_LINE_INDEX_LAST];

#endif /* __HMI_MPMCM_H__ */
