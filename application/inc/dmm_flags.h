/*
 * dmm_flags.h
 *
 *  Created on: 07 jan. 2023
 *      Author: Ludo
 */

#ifndef __DMM_FLAGS_H__
#define __DMM_FLAGS_H__

/*** Board modes ***/

//#define DMM_DEBUG
//#define DMM_NVM_FACTORY_RESET

/*** Board options ***/

#ifdef DMM_NVM_FACTORY_RESET
#define DMM_NODES_SCAN_PERIOD_SECONDS   86400
#define DMM_SIGFOX_UL_PERIOD_SECONDS    300
#define DMM_SIGFOX_DL_PERIOD_SECONDS    21600
#endif

#endif /* __DMM_FLAGS_H__ */
