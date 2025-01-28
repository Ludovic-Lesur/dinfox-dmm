/*
 * gpsm.c
 *
 *  Created on: 16 apr. 2023
 *      Author: Ludo
 */

#include "hmi_gpsm.h"

#include "common_registers.h"
#include "gpsm_registers.h"
#include "hmi_common.h"
#include "una.h"

/*** GPSM global variables ***/

const HMI_NODE_line_t HMI_GPSM_LINE[HMI_GPSM_LINE_INDEX_LAST] = {
    HMI_COMMON_LINE
    { "VGPS =", HMI_NODE_DATA_TYPE_VOLTAGE, GPSM_REGISTER_ADDRESS_ANALOG_DATA_1, GPSM_REGISTER_ANALOG_DATA_1_MASK_VGPS, GPSM_REGISTER_ADDRESS_CONTROL_1, UNA_REGISTER_MASK_NONE },
    { "VANT =", HMI_NODE_DATA_TYPE_VOLTAGE, GPSM_REGISTER_ADDRESS_ANALOG_DATA_1, GPSM_REGISTER_ANALOG_DATA_1_MASK_VANT, GPSM_REGISTER_ADDRESS_CONTROL_1, UNA_REGISTER_MASK_NONE },
};
