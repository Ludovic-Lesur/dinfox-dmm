/*
 * sh1106.h
 *
 *  Created on: 08 jan. 2023
 *      Author: Ludo
 */

#ifndef __SH1106_H__
#define __SH1106_H__

#include "i2c.h"
#include "font.h"
#include "string.h"
#include "types.h"

/*** SH1106 macros ***/

#define SH1106_HMI_I2C_ADDRESS			0x3C

#define SH1106_SCREEN_WIDTH_PIXELS		128
#define SH1106_SCREEN_WIDTH_CHAR		(SH1106_SCREEN_WIDTH_PIXELS / FONT_CHAR_WIDTH_PIXELS)
#define SH1106_SCREEN_HEIGHT_PIXELS		64
#define SH1106_SCREEN_HEIGHT_LINE		(SH1106_SCREEN_HEIGHT_PIXELS / 8)

/*** SH1106 structures ***/

/*!******************************************************************
 * \enum SH1106_status_t
 * \brief SH1106 driver error codes.
 *******************************************************************/
typedef enum {
	// Driver errors.
	SH1106_SUCCESS = 0,
	SH1106_ERROR_NULL_PARAMETER,
	SH1106_ERROR_DATA_TYPE,
	SH1106_ERROR_I2C_BUFFER_SIZE,
	SH1106_ERROR_PAGE_ADDRESS,
	SH1106_ERROR_COLUMN_ADDRESS,
	SH1106_ERROR_LINE_ADDRESS,
	SH1106_ERROR_CONTRAST,
	SH1106_ERROR_VERTICAL_POSITION,
	SH1106_ERROR_FLUSH_WIDTH_OVERFLOW,
	SH1106_ERROR_TEXT_WIDTH_OVERFLOW,
	SH1106_ERROR_HORIZONTAL_LINE_WIDTH,
	// Low level drivers errors.
	SH1106_ERROR_BASE_I2C1 = 0x0100,
	SH1106_ERROR_BASE_STRING = (SH1106_ERROR_BASE_I2C1 + I2C_ERROR_BASE_LAST),
	// Last base value.
	SH1106_ERROR_BASE_LAST = (SH1106_ERROR_BASE_STRING + STRING_ERROR_BASE_LAST)
} SH1106_status_t;

/*!******************************************************************
 * \enum SH1106_text_contrast_t
 * \brief SH1106 text contrast modes.
 *******************************************************************/
typedef enum {
	SH1106_TEXT_CONTRAST_NORMAL = 0,
	SH1106_TEXT_CONTRAST_INVERTED,
	SH1106_TEXT_CONTRAST_LAST
} SH1106_text_contrast_t;

/*!******************************************************************
 * \enum SH1106_text_vertical_position
 * \brief SH1106 text vertical position modes.
 *******************************************************************/
typedef enum {
	SH1106_TEXT_VERTICAL_POSITION_TOP = 0,
	SH1106_TEXT_VERTICAL_POSITION_BOTTOM,
	SH1106_TEXT_VERTICAL_POSITION_LAST
} SH1106_text_vertical_position;

/*!******************************************************************
 * \enum SH1106_text_t
 * \brief SH1106 text structure.
 *******************************************************************/
typedef struct {
	char_t* str;
	uint8_t page;
	STRING_justification_t justification;
	SH1106_text_contrast_t contrast;
	SH1106_text_vertical_position vertical_position;
	uint8_t flush_width_pixels;
} SH1106_text_t;

/*!******************************************************************
 * \enum SH1106_horizontal_line_t
 * \brief SH1106 horizontal line structure.
 *******************************************************************/
typedef struct {
	uint8_t line_pixels;
	uint8_t width_pixels;
	STRING_justification_t justification;
	SH1106_text_contrast_t contrast;
	uint8_t flush_flag;
} SH1106_horizontal_line_t;

/*** SH1106 functions ***/

/*!******************************************************************
 * \fn SH1106_status_t SH1106_setup(uint8_t i2c_address)
 * \brief Configure OLED screen.
 * \param[in]  	i2c_address: I2C address of the screen.
 * \param[out] 	none
 * \retval		Function execution status.
 *******************************************************************/
SH1106_status_t SH1106_setup(uint8_t i2c_address);

/*!******************************************************************
 * \fn SH1106_status_t SH1106_clear(uint8_t i2c_address)
 * \brief Clear OLED screen.
 * \param[in]  	i2c_address: I2C address of the screen.
 * \param[out] 	none
 * \retval		Function execution status.
 *******************************************************************/
SH1106_status_t SH1106_clear(uint8_t i2c_address);

/*!******************************************************************
 * \fn SH1106_status_t SH1106_print_text(uint8_t i2c_address, SH1106_text_t* text)
 * \brief Print text on OLED screen.
 * \param[in]	i2c_address: I2C address of the screen.
 * \param[in]  	text: Pointer to the text to print.
 * \param[out] 	none
 * \retval		Function execution status.
 *******************************************************************/
SH1106_status_t SH1106_print_text(uint8_t i2c_address, SH1106_text_t* text);

/*!******************************************************************
 * \fn SH1106_status_t SH1106_print_horizontal_line(uint8_t i2c_address, SH1106_horizontal_line_t* horizontal_line)
 * \brief Print a line on OLED screen.
 * \param[in]	i2c_address: I2C address of the screen.
 * \param[in]  	text: Pointer to the line to print.
 * \param[out] 	none
 * \retval		Function execution status.
 *******************************************************************/
SH1106_status_t SH1106_print_horizontal_line(uint8_t i2c_address, SH1106_horizontal_line_t* horizontal_line);

/*!******************************************************************
 * \fn SH1106_status_t SH1106_print_image(uint8_t i2c_address, const uint8_t image[SH1106_SCREEN_HEIGHT_LINE][SH1106_SCREEN_WIDTH_PIXELS])
 * \brief Print an image on OLED screen.
 * \param[in]	i2c_address: I2C address of the screen.
 * \param[in]  	image: Image bitmap to print.
 * \param[out] 	none
 * \retval		Function execution status.
 *******************************************************************/
SH1106_status_t SH1106_print_image(uint8_t i2c_address, const uint8_t image[SH1106_SCREEN_HEIGHT_LINE][SH1106_SCREEN_WIDTH_PIXELS]);

/*******************************************************************/
#define SH1106_exit_error(error_base) { if (sh1106_status != SH1106_SUCCESS) { status = (error_base + sh1106_status); goto errors; } }

/*******************************************************************/
#define SH1106_stack_error(void) { if (sh1106_status != SH1106_SUCCESS) { ERROR_stack_add(ERROR_BASE_SH1106 + sh1106_status); } }

/*******************************************************************/
#define SH1106_stack_exit_error(error_code) { if (sh1106_status != SH1106_SUCCESS) { ERROR_stack_add(ERROR_BASE_SH1106 + sh1106_status); status = error_code; goto errors; } }

#endif /* __SH1106_H__ */
