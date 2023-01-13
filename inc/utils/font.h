/*
 * font.h
 *
 *  Created on: 12 jan. 2023
 *      Author: Ludo
 */

#ifndef __FONT_H__
#define __FONT_H__

/*** FONT macros ***/

#define FONT_CHAR_WIDTH_PIXELS	6

/*** FONT characters bitmap definition ***/

static const uint8_t FONT_CHAR_L[FONT_CHAR_WIDTH_PIXELS] = {0xFF, 0x80, 0x80, 0x80, 0x80, 0x00};
static const uint8_t FONT_CHAR_u[FONT_CHAR_WIDTH_PIXELS] = {0xF0, 0x80, 0x80, 0x80, 0xF0, 0x00};
static const uint8_t FONT_CHAR_d[FONT_CHAR_WIDTH_PIXELS] = {0xF0, 0x90, 0x90, 0x90, 0xFF, 0x00};
static const uint8_t FONT_CHAR_o[FONT_CHAR_WIDTH_PIXELS] = {0xF0, 0x90, 0x90, 0x90, 0xF0, 0x00};



#endif /* __FONT_H__ */
