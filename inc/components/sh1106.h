/*
 * sh1106.h
 *
 *  Created on: 08 jan. 2023
 *      Author: Ludo
 */

#ifndef __SH1106_H__
#define __SH1106_H__

#include "i2c.h"

/*** SH1106 macros ***/

#define SH1106_I2C_ADDRESS	0x3C

/*** SH1106 structures ***/

typedef enum {
	SH1106_SUCCESS,
	SH1106_ERROR_DATA_TYPE,
	SH1106_ERROR_I2C_BUFFER_SIZE,
	SH1106_ERROR_PAGE_ADDRESS,
	SH1106_ERROR_COLUMN_ADDRESS,
	SH1106_ERROR_LINE_ADDRESS,
	SH1106_ERROR_BASE_I2C = 0x0100,
	SH1106_ERROR_BASE_LAST = (SH1106_ERROR_BASE_I2C + I2C_ERROR_BASE_LAST)
} SH1106_status_t;

/*** SH1106 functions ***/

SH1106_status_t SH1106_init(void);
SH1106_status_t SH1106_print_image(const uint8_t image[8][128]);

#define SH1106_status_check(error_base) { if (sh1106_status != SH1106_SUCCESS) { status = error_base + sh1106_status; goto errors; }}
#define SH1106_error_check() { ERROR_status_check(sh1106_status, SH1106_SUCCESS, ERROR_BASE_SH1106); }
#define SH1106_error_check_print() { ERROR_status_check_print(sh1106_status, SH1106_SUCCESS, ERROR_BASE_SH1106); }

#endif /* __SH1106_H__ */
