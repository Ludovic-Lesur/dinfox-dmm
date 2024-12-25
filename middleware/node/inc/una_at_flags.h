/*
 * una_at_flags.h
 *
 *  Created on: 07 dec. 2024
 *      Author: Ludo
 */

#include "lmac.h"
#include "lptim.h"
#include "terminal_instance.h"

#ifndef __UNA_AT_FLAGS_H__
#define __UNA_AT_FLAGS_H__

/*** UNA AT compilation flags ***/

#define UNA_AT_DELAY_ERROR_BASE_LAST        LPTIM_ERROR_BASE_LAST

#define UNA_AT_MODE_MASTER
//#define UNA_AT_MODE_SLAVE

#define UNA_AT_USE_REGISTER_ACCESS
#define UNA_AT_USE_REGISTER_ACCESS_TIMEOUT
#define UNA_AT_USE_REGISTER_ERROR_VALUE

#define UNA_AT_TERMINAL_INSTANCE            TERMINAL_INSTANCE_LMAC

#endif /* __UNA_AT_FLAGS_H__ */
