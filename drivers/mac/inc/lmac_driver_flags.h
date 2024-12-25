/*
 * lmac_driver_flags.h
 *
 *  Created on: 27 nov. 2024
 *      Author: Ludo
 */

#ifndef __LMAC_DRIVER_FLAGS_H__
#define __LMAC_DRIVER_FLAGS_H__

#include "lpuart.h"
#include "nvm.h"

/*** LMAC driver compilation flags ***/

#define LMAC_DRIVER_HW_INTERFACE_ERROR_BASE_LAST    LPUART_ERROR_BASE_LAST
#define LMAC_DRIVER_NVM_ERROR_BASE_LAST             NVM_ERROR_BASE_LAST

#define LMAC_DRIVER_MODE_MASTER
//#define LMAC_DRIVER_MODE_SLAVE

#ifdef LMAC_DRIVER_MODE_SLAVE
#define LMAC_DRIVER_RX_TIMEOUT_SECONDS              5
#endif

#endif /* __LMAC_DRIVER_FLAGS_H__ */
