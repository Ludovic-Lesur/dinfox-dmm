/*
 * mode.h
 *
 *  Created on: 07 jan. 2023
 *      Author: Ludo
 */

#ifndef __MODE_H__
#define __MODE_H__

/*** DMM optional features ***/

// Enable battery security with LVRM (macros gives the node address).
//#define BMS
#ifdef BMS
#define BMS_NODE_ADDRESS				0x20
#define BMS_MONITORING_PERIOD_SECONDS	60
#define BMS_VBATT_LOW_THRESHOLD_MV		10000
#define BMS_VBATT_HIGH_THRESHOLD_MV		12000
#endif

/*** Debug mode ***/

//#define DEBUG		// Use programming pins for debug purpose if defined.

#endif /* __MODE_H__ */
