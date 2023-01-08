/*
 * dmm.h
 *
 *  Created on: 08 jan. 2023
 *      Author: Ludo
 */

#ifndef __DMM_H__
#define __DMM_H__

#include "dinfox.h"

typedef enum {
	DMM_REGISTER_VUSB_MV = DINFOX_REGISTER_LAST,
	DMM_REGISTER_VRS_MV,
	DMM_REGISTER_VHMI_MV,
	DMM_REGISTER_LAST,
} DMM_register_address_t;

#endif /* __DMM_H__ */
