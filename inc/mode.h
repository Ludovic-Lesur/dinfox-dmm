/*
 * mode.h
 *
 *  Created on: 07 jan. 2023
 *      Author: Ludo
 */

#ifndef __MODE_H__
#define __MODE_H__

/*** DMM options ***/

// Default periods.
#define DMM_SIGFOX_UL_PERIOD_SECONDS_DEFAULT	300
#define DMM_SIGFOX_DL_PERIOD_SECONDS_DEFAULT	21600
#define DMM_NODES_SCAN_PERIOD_SECONDS_DEFAULT	86400

// Battery security system with LVRM.
//#define DMM_BMS_ENABLE
#define DMM_BMS_NODE_ADDRESS					0x20
#define DMM_BMS_MONITORING_PERIOD_SECONDS		60
#define DMM_BMS_VBATT_LOW_THRESHOLD_MV			10000
#define DMM_BMS_VBATT_HIGH_THRESHOLD_MV			12000

// Debug mode.
//#define DEBUG

#endif /* __MODE_H__ */
