/*
 * font.h
 *
 *  Created on: 12 jan. 2023
 *      Author: Ludo
 */

#ifndef __FONT_H__
#define __FONT_H__

#include "types.h"

/*** FONT macros ***/

#define FONT_ASCII_TABLE_SIZE			128
#define FONT_ASCII_TABLE_OFFSET			32
#define FONT_TABLE_SIZE					(FONT_ASCII_TABLE_SIZE - FONT_ASCII_TABLE_OFFSET)
#define FONT_CHAR_WIDTH_PIXELS			6

/*** FONT global variables ***/

extern const uint8_t FONT[FONT_TABLE_SIZE][FONT_CHAR_WIDTH_PIXELS];

#endif /* __FONT_H__ */
