/*
 * una_at_flags.h
 *
 *  Created on: 07 dec. 2024
 *      Author: Ludo
 */

#ifndef __UNA_AT_FLAGS_H__
#define __UNA_AT_FLAGS_H__

#include "common_registers.h"
#include "lptim.h"
#include "terminal_instance.h"

/*** UNA AT compilation flags ***/

#define UNA_AT_DELAY_ERROR_BASE_LAST            LPTIM_ERROR_BASE_LAST

#define UNA_AT_TERMINAL_INSTANCE                TERMINAL_INSTANCE_LMAC

#define UNA_AT_MODE_MASTER
//#define UNA_AT_MODE_SLAVE

#ifdef UNA_AT_MODE_MASTER

#define UNA_AT_NODE_ACCESS_RETRY_MAX            3
#define UNA_AT_SCAN_REGISTER_ADDRESS            COMMON_REGISTER_ADDRESS_NODE_ID
#define UNA_AT_SCAN_REGISTER_MASK_NODE_ADDRESS  COMMON_REGISTER_NODE_ID_MASK_NODE_ADDR
#define UNA_AT_SCAN_REGISTER_MASK_BOARD_ID      COMMON_REGISTER_NODE_ID_MASK_BOARD_ID
#define UNA_AT_SCAN_REGISTER_TIMEOUT_MS         200

#endif /* UNA_AT_MODE_MASTER */

#ifdef UNA_AT_MODE_SLAVE

//#define UNA_AT_CUSTOM_COMMANDS

#endif /* UNA_AT_MODE_SLAVE */

#endif /* __UNA_AT_FLAGS_H__ */
