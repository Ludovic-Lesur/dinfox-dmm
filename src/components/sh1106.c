/*
 * sh1106.c
 *
 *  Created on: 08 jan. 2023
 *      Author: Ludo
 */

#include "sh1106.h"

#include "error.h"
#include "font.h"
#include "i2c.h"
#include "lptim.h"
#include "types.h"

/*** SH1106 local macros ***/

#define SH1106_COLUMN_ADDRESS_LAST		131
#define SH1106_LINE_ADDRESS_LAST		63

#define SH1106_RAM_WIDTH_PIXELS			(SH1106_COLUMN_ADDRESS_LAST + 1)
#define SH1106_RAM_HEIGHT_PIXELS		(SH1106_LINE_ADDRESS_LAST + 1)

#define SH1106_OFFSET_WIDTH_PIXELS		((SH1106_RAM_WIDTH_PIXELS - SH1106_SCREEN_WIDTH_PIXELS) / 2)
#define SH1106_OFFSET_HEIGHT_PIXELS		((SH1106_RAM_HEIGHT_PIXELS - SH1106_SCREEN_HEIGHT_PIXELS) / 2)

#define SH1106_I2C_BUFFER_SIZE_BYTES	256

/*** SH1106 local structures ***/

/*******************************************************************/
typedef enum {
	SH1106_DATA_TYPE_COMMAND = 0,
	SH1106_DATA_TYPE_RAM,
	SH1106_DATA_TYPE_LAST
} SH1106_data_type_t;

/*******************************************************************/
typedef struct {
	uint8_t i2c_tx_buffer[SH1106_I2C_BUFFER_SIZE_BYTES];
	uint8_t i2c_rx_buffer[SH1106_I2C_BUFFER_SIZE_BYTES];
	uint8_t ram_data[SH1106_SCREEN_WIDTH_PIXELS];
} SH1106_context_t;

/*** SH1106 local global variables ***/

static SH1106_context_t sh1106_ctx;

/*** SH1106 local functions ***/

/*******************************************************************/
SH1106_status_t _SH1106_write(uint8_t i2c_address, SH1106_data_type_t data_type, uint8_t* data, uint8_t data_size_bytes) {
	// Local variables.
	SH1106_status_t status = SH1106_SUCCESS;
	I2C_status_t i2c1_status = I2C_SUCCESS;
	uint8_t idx = 0;
	// Check parameters.
	if (data_type >= SH1106_DATA_TYPE_LAST) {
		status = SH1106_ERROR_DATA_TYPE;
		goto errors;
	}
	if (data_size_bytes >= SH1106_I2C_BUFFER_SIZE_BYTES) {
		status = SH1106_ERROR_I2C_BUFFER_SIZE;
		goto errors;
	}
	// Build TX buffer.
	sh1106_ctx.i2c_tx_buffer[0] = (data_type == SH1106_DATA_TYPE_COMMAND) ? 0x00 : 0x40;
	for (idx=0 ; idx<data_size_bytes ; idx++) {
		sh1106_ctx.i2c_tx_buffer[idx + 1] = data[idx];
	}
	// Burst write with C0='0'.
	i2c1_status = I2C1_write(i2c_address, sh1106_ctx.i2c_tx_buffer, (data_size_bytes + 1), 1);
	I2C1_check_status(SH1106_ERROR_BASE_I2C);
errors:
	return status;
}

/*******************************************************************/
SH1106_status_t _SH1106_read(uint8_t i2c_address, uint8_t* data, uint8_t data_size_bytes) {
	// Local variables.
	SH1106_status_t status = SH1106_SUCCESS;
	I2C_status_t i2c1_status = I2C_SUCCESS;
	// Read data.
	i2c1_status = I2C1_read(i2c_address, data, data_size_bytes);
	I2C1_check_status(SH1106_ERROR_BASE_I2C);
errors:
	return status;
}

/*******************************************************************/
SH1106_status_t _SH1106_setup(uint8_t i2c_address) {
	// Local variables.
	SH1106_status_t status = SH1106_SUCCESS;
	const uint8_t command_list_size = 2;
	uint8_t command_list[command_list_size];
	// Build commands.
	command_list[0] = 0xA1; // Invert horizontal orientation.
	command_list[1] = 0xC8; // Invert vertical orientation.
	// Send command.
	status = _SH1106_write(i2c_address, SH1106_DATA_TYPE_COMMAND, command_list, command_list_size);
	if (status != SH1106_SUCCESS) goto errors;
errors:
	return status;
}

/*******************************************************************/
SH1106_status_t _SH1106_set_address(uint8_t i2c_address, uint8_t page, uint8_t column, uint8_t line) {
	// Local variables.
	SH1106_status_t status = SH1106_SUCCESS;
	const uint8_t command_list_size = 4;
	uint8_t command_list[command_list_size];
	// Check parameters.
	if (page > SH1106_SCREEN_HEIGHT_LINE) {
		status = SH1106_ERROR_PAGE_ADDRESS;
		goto errors;
	}
	if (column > SH1106_SCREEN_WIDTH_PIXELS) {
		status = SH1106_ERROR_COLUMN_ADDRESS;
		goto errors;
	}
	if (line > SH1106_SCREEN_HEIGHT_PIXELS) {
		status = SH1106_ERROR_LINE_ADDRESS;
		goto errors;
	}
	// Build commands.
	command_list[0] = 0xB0 | (page & 0x0F);
	command_list[1] = 0x00 | (((column + SH1106_OFFSET_WIDTH_PIXELS) >> 0) & 0x0F);
	command_list[2] = 0x10 | (((column + SH1106_OFFSET_WIDTH_PIXELS) >> 4) & 0x0F);
	command_list[3] = 0x40 | ((line + SH1106_OFFSET_HEIGHT_PIXELS) & 0x3F);
	status = _SH1106_write(i2c_address, SH1106_DATA_TYPE_COMMAND, command_list, command_list_size);
	if (status != SH1106_SUCCESS) goto errors;
errors:
	return status;
}

/*******************************************************************/
SH1106_status_t _SH1106_clear_ram(uint8_t i2c_address) {
	// Local variables.
	SH1106_status_t status = SH1106_SUCCESS;
	uint8_t idx = 0;
	// Reset data.
	for (idx=0 ; idx<SH1106_RAM_WIDTH_PIXELS ; idx++) sh1106_ctx.ram_data[idx] = 0x00;
	// Page loop.
	for (idx=0 ; idx<SH1106_SCREEN_HEIGHT_LINE ; idx++) {
		// Clear RAM page.
		status = _SH1106_set_address(i2c_address, idx, 0, 0);
		if (status != SH1106_SUCCESS) goto errors;
		status = _SH1106_write(i2c_address, SH1106_DATA_TYPE_RAM, sh1106_ctx.ram_data, SH1106_RAM_WIDTH_PIXELS);
		if (status != SH1106_SUCCESS) goto errors;
	}
errors:
	return status;
}

/*******************************************************************/
SH1106_status_t _SH1106_on_off(uint8_t i2c_address, uint8_t on_off_flag) {
	// Local variables.
	SH1106_status_t status = SH1106_SUCCESS;
	uint8_t command = 0;
	// Build command.
	command = 0xAE | (on_off_flag & 0x01);
	// Send command.
	status = _SH1106_write(i2c_address, SH1106_DATA_TYPE_COMMAND, &command, 1);
	if (status != SH1106_SUCCESS) goto errors;
errors:
	return status;
}

/*** SH1106 functions ***/

/*******************************************************************/
SH1106_status_t SH1106_setup(uint8_t i2c_address) {
	// Local variables.
	SH1106_status_t status = SH1106_SUCCESS;
	// Screen configuration.
	status = _SH1106_setup(i2c_address);
	if (status != SH1106_SUCCESS) goto errors;
	// Clear RAM.
	status = _SH1106_clear_ram(i2c_address);
	if (status != SH1106_SUCCESS) goto errors;
errors:
	return status;
}

/*******************************************************************/
SH1106_status_t SH1106_clear(uint8_t i2c_address) {
	// Local variables.
	SH1106_status_t status = SH1106_SUCCESS;
	// Turn display off.
	status = _SH1106_on_off(i2c_address, 0);
	if (status != SH1106_SUCCESS) goto errors;
	// Clear RAM.
	status = _SH1106_clear_ram(i2c_address);
	if (status != SH1106_SUCCESS) goto errors;
errors:
	return status;
}

/*******************************************************************/
SH1106_status_t SH1106_print_text(uint8_t i2c_address, SH1106_text_t* text) {
	// Local variables.
	SH1106_status_t status = SH1106_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	uint8_t ram_idx = 0;
	uint8_t text_size = 0;
	uint8_t text_width_pixels = 0;
	uint8_t text_column = 0;
	uint8_t flush_column = 0;
	uint8_t text_idx = 0;
	uint8_t line_idx = 0;
	uint8_t ascii_code = 0;
	// Check parameter.
	if (text == NULL) {
		status = SH1106_ERROR_NULL_PARAMETER;
		goto errors;
	}
	if ((text -> str) == NULL) {
		status = SH1106_ERROR_NULL_PARAMETER;
		goto errors;
	}
	if ((text -> contrast) >= SH1106_TEXT_CONTRAST_LAST) {
		status = SH1106_ERROR_CONTRAST;
		goto errors;
	}
	if ((text -> vertical_position) >= SH1106_TEXT_VERTICAL_POSITION_LAST) {
		status = SH1106_ERROR_CONTRAST;
		goto errors;
	}
	if ((text -> flush_width_pixels) > SH1106_SCREEN_WIDTH_PIXELS) {
		status = SH1106_ERROR_FLUSH_WIDTH_OVERFLOW;
		goto errors;
	}
	// Compute text width.
	string_status = STRING_get_size((text -> str), &text_size);
	STRING_check_status(SH1106_ERROR_BASE_STRING);
	text_width_pixels = (text_size * FONT_CHAR_WIDTH_PIXELS);
	// Check size.
	if (text_width_pixels > SH1106_SCREEN_WIDTH_PIXELS) {
		status = SH1106_ERROR_TEXT_WIDTH_OVERFLOW;
		goto errors;
	}
	// Compute column indexes according to justification.
	switch (text -> justification) {
	case STRING_JUSTIFICATION_LEFT:
		text_column = 0;
		flush_column = 0;
		break;
	case STRING_JUSTIFICATION_CENTER:
		text_column = (SH1106_SCREEN_WIDTH_PIXELS - text_width_pixels) / (2);
		flush_column = (SH1106_SCREEN_WIDTH_PIXELS - (text -> flush_width_pixels)) / (2);
		break;
	case STRING_JUSTIFICATION_RIGHT:
		text_column = (SH1106_SCREEN_WIDTH_PIXELS - text_width_pixels);
		flush_column = (SH1106_SCREEN_WIDTH_PIXELS - (text -> flush_width_pixels));
		break;
	default:
		status = (HMI_ERROR_BASE_STRING + STRING_ERROR_TEXT_JUSTIFICATION);
		goto errors;
	}
	// Reset RAM data.
	for (ram_idx=0 ; ram_idx<SH1106_RAM_WIDTH_PIXELS ; ram_idx++) sh1106_ctx.ram_data[ram_idx] = 0x00;
	// Build RAM data.
	ram_idx = text_column;
	text_idx = 0;
	while ((text -> str)[text_idx] != STRING_CHAR_NULL) {
		// Get ASCII code.
		ascii_code = (uint8_t) (text -> str)[text_idx];
		// Line loop.
		for (line_idx=0 ; line_idx<FONT_CHAR_WIDTH_PIXELS ; line_idx++) {
			// Check index.
			if (ram_idx >= SH1106_SCREEN_WIDTH_PIXELS) {
				status = SH1106_ERROR_TEXT_WIDTH_OVERFLOW;
				goto errors;
			}
			// Fill RAM.
			sh1106_ctx.ram_data[ram_idx] = (ascii_code < FONT_ASCII_TABLE_OFFSET) ? FONT[0][line_idx] : FONT[ascii_code - FONT_ASCII_TABLE_OFFSET][line_idx];
			if ((text -> vertical_position) == SH1106_TEXT_VERTICAL_POSITION_BOTTOM) {
				sh1106_ctx.ram_data[ram_idx] <<= 1;
			}
			ram_idx++;
		}
		text_idx++;
	}
	// Manage contrast.
	if ((text -> contrast) == SH1106_TEXT_CONTRAST_INVERTED) {
		for (ram_idx=0 ; ram_idx<SH1106_SCREEN_WIDTH_PIXELS ; ram_idx++) sh1106_ctx.ram_data[ram_idx] ^= 0xFF;
	}
	// Check flush width.
	if ((text -> flush_width_pixels) == 0) {
		// Set address.
		status = _SH1106_set_address(i2c_address, (text -> page), text_column, 0);
		if (status != SH1106_SUCCESS) goto errors;
		// Write line.
		status = _SH1106_write(i2c_address, SH1106_DATA_TYPE_RAM, &(sh1106_ctx.ram_data[text_column]), text_width_pixels);
		if (status != SH1106_SUCCESS) goto errors;
	}
	else {
		// Set address.
		status = _SH1106_set_address(i2c_address, (text -> page), flush_column, 0);
		if (status != SH1106_SUCCESS) goto errors;
		// Write line.
		status = _SH1106_write(i2c_address, SH1106_DATA_TYPE_RAM, &(sh1106_ctx.ram_data[flush_column]), (text -> flush_width_pixels));
		if (status != SH1106_SUCCESS) goto errors;
	}
	// Turn display on.
	status = _SH1106_on_off(i2c_address, 1);
	if (status != SH1106_SUCCESS) goto errors;
errors:
	return status;
}

/*******************************************************************/
SH1106_status_t SH1106_print_horizontal_line(uint8_t i2c_address, SH1106_horizontal_line_t* horizontal_line) {
	// Local variables.
	SH1106_status_t status = SH1106_SUCCESS;
	uint8_t line_column = 0;
	uint8_t ram_idx = 0;
	// Check parameters.
	if (horizontal_line == NULL) {
		status = SH1106_ERROR_NULL_PARAMETER;
		goto errors;
	}
	if ((horizontal_line -> line_pixels) >= SH1106_SCREEN_HEIGHT_PIXELS) {
		status = SH1106_ERROR_LINE_ADDRESS;
		goto errors;
	}
	if ((horizontal_line -> width_pixels) > SH1106_SCREEN_WIDTH_PIXELS) {
		status = SH1106_ERROR_HORIZONTAL_LINE_WIDTH;
		goto errors;
	}
	// Compute column according to justification.
	switch (horizontal_line -> justification) {
	case STRING_JUSTIFICATION_LEFT:
		line_column = 0;
		break;
	case STRING_JUSTIFICATION_CENTER:
		line_column = (SH1106_SCREEN_WIDTH_PIXELS - (horizontal_line -> width_pixels)) / (2);
		break;
	case STRING_JUSTIFICATION_RIGHT:
		line_column = (SH1106_SCREEN_WIDTH_PIXELS - (horizontal_line -> width_pixels));
		break;
	default:
		status = (HMI_ERROR_BASE_STRING + STRING_ERROR_TEXT_JUSTIFICATION);
		goto errors;
	}
	// Build RAM data.
	for (ram_idx=0 ; ram_idx<SH1106_RAM_WIDTH_PIXELS ; ram_idx++) {
		sh1106_ctx.ram_data[ram_idx] = ((ram_idx >= line_column) && (ram_idx < (line_column + (horizontal_line -> width_pixels)))) ? (0b1 << ((horizontal_line -> line_pixels) % 8)) : 0x00;
	}
	// Manage contrast.
	if ((horizontal_line -> contrast) == SH1106_TEXT_CONTRAST_INVERTED) {
		for (ram_idx=0 ; ram_idx<SH1106_SCREEN_WIDTH_PIXELS ; ram_idx++) sh1106_ctx.ram_data[ram_idx] ^= 0xFF;
	}
	// Check line erase flag.
	if ((horizontal_line -> flush_flag) != 0) {
		// Set address.
		status = _SH1106_set_address(i2c_address, ((horizontal_line -> line_pixels) / 8), 0, 0);
		if (status != SH1106_SUCCESS) goto errors;
		// Write line.
		status = _SH1106_write(i2c_address, SH1106_DATA_TYPE_RAM, sh1106_ctx.ram_data, SH1106_SCREEN_WIDTH_PIXELS);
		if (status != SH1106_SUCCESS) goto errors;
	}
	else {
		// Set address.
		status = _SH1106_set_address(i2c_address, ((horizontal_line -> line_pixels) / 8), line_column, 0);
		if (status != SH1106_SUCCESS) goto errors;
		// Write line.
		status = _SH1106_write(i2c_address, SH1106_DATA_TYPE_RAM, &(sh1106_ctx.ram_data[line_column]), (horizontal_line -> width_pixels));
		if (status != SH1106_SUCCESS) goto errors;
	}
errors:
	return status;
}

/*******************************************************************/
SH1106_status_t SH1106_print_image(uint8_t i2c_address, const uint8_t image[SH1106_SCREEN_HEIGHT_LINE][SH1106_SCREEN_WIDTH_PIXELS]) {
	// Local variables.
	SH1106_status_t status = SH1106_SUCCESS;
	uint8_t page = 0;
	// Page loop.
	for (page=0 ; page<SH1106_SCREEN_HEIGHT_LINE ; page++) {
		// Display line.
		status = _SH1106_set_address(i2c_address, page, 0, 0);
		if (status != SH1106_SUCCESS) goto errors;
		status = _SH1106_write(i2c_address, SH1106_DATA_TYPE_RAM, (uint8_t*) image[page], SH1106_SCREEN_WIDTH_PIXELS);
		if (status != SH1106_SUCCESS) goto errors;
	}
	// Turn display on.
	status = _SH1106_on_off(i2c_address, 1);
	if (status != SH1106_SUCCESS) goto errors;
errors:
	return status;
}
