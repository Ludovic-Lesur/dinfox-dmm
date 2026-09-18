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
#define DMM_NODE_SCAN_PERIOD_SECONDS    (1 * MATH_SECONDS_PER_DAY)
#define DMM_SIGFOX_UL_PERIOD_SECONDS    (5 * MATH_SECONDS_PER_MINUTE)
#define DMM_SIGFOX_DL_PERIOD_SECONDS    (6 * MATH_SECONDS_PER_HOUR)
#endif

#endif /* __DMM_FLAGS_H__ */
