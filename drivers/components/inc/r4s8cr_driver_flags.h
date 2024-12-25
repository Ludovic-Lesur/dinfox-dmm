/*
 * r4s8cr_driver_flags.h
 *
 *  Created on: 28 nov. 2024
 *      Author: Ludo
 */

#ifndef __R4S8CR_DRIVER_FLAGS_H__
#define __R4S8CR_DRIVER_FLAGS_H__

#include "lptim.h"
#include "lpuart.h"

/*** R4S8CR driver compilation flags ***/

#define R4S8CR_DRIVER_RS485_ERROR_BASE_LAST     LPUART_ERROR_BASE_LAST
#define R4S8CR_DRIVER_DELAY_ERROR_BASE_LAST     LPTIM_ERROR_BASE_LAST

#endif /* __R4S8CR_DRIVER_FLAGS_H__ */
