/*
 * nvm.h
 *
 *  Created on: 19 june 2018
 *      Author: Ludo
 */

#ifndef __NVM_H__
#define __NVM_H__

#include "types.h"

/*** NVM macros ***/

typedef enum {
	NVM_SUCCESS = 0,
	NVM_ERROR_NULL_PARAMETER,
	NVM_ERROR_ADDRESS,
	NVM_ERROR_UNLOCK,
	NVM_ERROR_LOCK,
	NVM_ERROR_WRITE,
	NVM_ERROR_BASE_LAST = 0x0100
} NVM_status_t;

typedef enum {
	NVM_ADDRESS_RS485_ADDRESS = 0,
	NVM_ADDRESS_LAST
} NVM_address_t;

/*** NVM functions ***/

void NVM_init(void);
NVM_status_t NVM_read_byte(NVM_address_t address_offset, uint8_t* data);
NVM_status_t NVM_write_byte(NVM_address_t address_offset, uint8_t data);

#define NVM_status_check(error_base) { if (nvm_status != NVM_SUCCESS) { status = error_base + nvm_status; goto errors; }}
#define NVM_error_check() { ERROR_status_check(nvm_status, NVM_SUCCESS, ERROR_BASE_NVM); }
#define NVM_error_check_print() { ERROR_status_check_print(nvm_status, NVM_SUCCESS, ERROR_BASE_NVM); }

#endif /* __NVM_H__ */
