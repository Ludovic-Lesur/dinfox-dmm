/*
 * hmi.c
 *
 *  Created on: 14 jan. 2023
 *      Author: Ludo
 */

#include "hmi.h"

#include "dinfox.h"
#include "error.h"
#include "exti.h"
#include "font.h"
#include "gpio.h"
#include "i2c.h"
#include "iwdg.h"
#include "led.h"
#include "logo.h"
#include "lpuart.h"
#include "mapping.h"
#include "node.h"
#include "nvic.h"
#include "pwr.h"
#include "sh1106.h"
#include "string.h"
#include "types.h"

/*** HMI macros ***/

#define HMI_DATA_PAGES_DISPLAYED				3
#define HMI_DATA_PAGES_DEPTH					32

#define HMI_UNUSED_DURATION_THRESHOLD_SECONDS	5

#define HMI_STRING_VALUE_BUFFER_SIZE			16

#define HMI_NAVIGATION_ZONE_WIDTH_CHAR			1
#define HMI_NAVIGATION_ZONE_WIDTH_PIXELS		(HMI_NAVIGATION_ZONE_WIDTH_CHAR * FONT_CHAR_WIDTH_PIXELS)

#define HMI_DATA_ZONE_WIDTH_CHAR				(SH1106_SCREEN_WIDTH_CHAR - (2 * (HMI_NAVIGATION_ZONE_WIDTH_CHAR + 1)))
#define HMI_DATA_ZONE_WIDTH_PIXELS				(HMI_DATA_ZONE_WIDTH_CHAR * FONT_CHAR_WIDTH_PIXELS)

#define HMI_SYMBOL_SELECT						'>'
#define HMI_SYMBOL_TOP							'\''
#define HMI_SYMBOL_BOTTOM						'`'

static const char_t* HMI_TITLE_NODES_LIST = "NODES LIST";

static const char_t* HMI_TEXT_ERROR = "ERROR";
static const char_t* HMI_TEXT_NA = "N/A";

static const char_t* HMI_MESSAGE_NODES_SCAN_RUNNING[HMI_DATA_PAGES_DISPLAYED] = {"NODES SCAN", "RUNNING", "..."};
static const char_t* HMI_MESSAGE_UNSUPPORTED_NODE[HMI_DATA_PAGES_DISPLAYED] = {"UNSUPPORTED", "NODE", STRING_NULL};
static const char_t* HMI_MESSAGE_NONE_MEASUREMENT[HMI_DATA_PAGES_DISPLAYED] = {"NONE", "MEASUREMENT", "ON THIS NODE"};
static const char_t* HMI_MESSAGE_READING_DATA[HMI_DATA_PAGES_DISPLAYED] = {"READING DATA", "...", STRING_NULL};

/*** HMI local structures ***/

typedef HMI_status_t (*HMI_irq_callback_t)(void);

typedef enum {
	HMI_STATE_INIT = 0,
	HMI_STATE_IDLE,
	HMI_STATE_UNUSED,
	HMI_STATE_LAST,
} HMI_state_t;

typedef enum {
	HMI_SCREEN_OFF = 0,
	HMI_SCREEN_NODES_LIST,
	HMI_SCREEN_NODES_SCAN,
	HMI_SCREEN_NODE_DATA,
	HMI_SCREEN_ERROR,
	HMI_SCREEN_LAST,
} HMI_screen_t;

typedef enum {
	HMI_PAGE_ADDRESS_TITLE = 0,
	HMI_PAGE_ADDRESS_SPACE_1,
	HMI_PAGE_ADDRESS_SPACE_2,
	HMI_PAGE_ADDRESS_DATA_1,
	HMI_PAGE_ADDRESS_SPACE_3,
	HMI_PAGE_ADDRESS_DATA_2,
	HMI_PAGE_ADDRESS_SPACE_4,
	HMI_PAGE_ADDRESS_DATA_3,
	HMI_PAGE_ADDRESS_LAST
} HMI_page_address_t;

typedef struct {
	// Global.
	HMI_status_t status;
	HMI_state_t state;
	HMI_screen_t screen;
	volatile uint8_t irq_flags;
	HMI_irq_callback_t irq_callbacks[HMI_IRQ_LAST];
	// Screen.
	char_t text[HMI_DATA_ZONE_WIDTH_CHAR + 1];
	uint8_t text_width;
	char_t data[HMI_DATA_PAGES_DEPTH][HMI_DATA_ZONE_WIDTH_CHAR + 1];
	uint8_t data_depth;
	uint8_t data_index;
	uint8_t data_offset_index;
	uint8_t pointer_index;
	char_t navigation_left[HMI_DATA_PAGES_DISPLAYED][HMI_NAVIGATION_ZONE_WIDTH_CHAR + 1];
	char_t navigation_right[HMI_DATA_PAGES_DISPLAYED][HMI_NAVIGATION_ZONE_WIDTH_CHAR + 1];
	SH1106_horizontal_line_t sh1106_line;
	// Current node.
	NODE_t node;
} HMI_context_t;

/*** HMI local global variables ***/

static HMI_context_t hmi_ctx;
static const uint8_t HMI_DATA_PAGE_ADDRESS[HMI_DATA_PAGES_DISPLAYED] = {HMI_PAGE_ADDRESS_DATA_1, HMI_PAGE_ADDRESS_DATA_2, HMI_PAGE_ADDRESS_DATA_3};

/*** HMI local functions ***/

/* FLUSH TEXT BUFFER.
 * @param:	None.
 * @return:	None.
 */
static void _HMI_text_flush(void) {
	// Local variables.
	uint8_t idx = 0;
	// Flush string.
	for (idx=0 ; idx<(HMI_DATA_ZONE_WIDTH_CHAR + 1) ; idx++) hmi_ctx.text[idx] = STRING_CHAR_NULL;
	hmi_ctx.text_width = 0;
}

/* FLUSH DATA BUFFER.
 * @param:	None.
 * @return:	None.
 */
static void _HMI_data_flush(void) {
	// Local variables.
	uint8_t line_idx = 0;
	uint8_t char_idx = 0;
	// Line loop.
	for (line_idx=0 ; line_idx<HMI_DATA_PAGES_DEPTH ; line_idx++) {
		// Char loop.
		for (char_idx=0 ; char_idx<(HMI_DATA_ZONE_WIDTH_CHAR + 1) ; char_idx++) {
			hmi_ctx.data[line_idx][char_idx] = STRING_CHAR_NULL;
		}
	}
	hmi_ctx.data_depth = 0;
}

/* PRINT TITLE ZONE ON SCREEN.
 * @param title:	None.
 * @return status:	Function executions status.
 */
static HMI_status_t _HMI_print_title(char_t* title) {
	// Local variables.
	HMI_status_t status = HMI_SUCCESS;
	SH1106_status_t sh1106_status = SH1106_SUCCESS;
	SH1106_text_t sh1106_text;
	// Title text parameters.
	sh1106_text.str = title;
	sh1106_text.page = HMI_PAGE_ADDRESS_TITLE;
	sh1106_text.justification = STRING_JUSTIFICATION_CENTER;
	sh1106_text.contrast = SH1106_TEXT_CONTRAST_INVERTED;
	sh1106_text.vertical_position = SH1106_TEXT_VERTICAL_POSITION_BOTTOM;
	sh1106_text.flush_width_pixels = SH1106_SCREEN_WIDTH_PIXELS;
	// Add line.
	hmi_ctx.sh1106_line.line_pixels = 8;
	hmi_ctx.sh1106_line.width_pixels = SH1106_SCREEN_WIDTH_PIXELS;
	hmi_ctx.sh1106_line.justification = STRING_JUSTIFICATION_CENTER;
	hmi_ctx.sh1106_line.contrast = SH1106_TEXT_CONTRAST_NORMAL;
	hmi_ctx.sh1106_line.flush_flag = 1;
	// Print title.
	sh1106_status = SH1106_print_text(&sh1106_text);
	SH1106_status_check(HMI_ERROR_BASE_SH1106);
	sh1106_status = SH1106_print_horizontal_line(&hmi_ctx.sh1106_line);
	SH1106_status_check(HMI_ERROR_BASE_SH1106);
errors:
	return status;
}

/* PRINT NAVIGATION ZONE ON SCREEN.
 * @param:			None.
 * @return status:	Function executions status.
 */
static HMI_status_t _HMI_print_navigation(void) {
	// Local variables.
	HMI_status_t status = HMI_SUCCESS;
	SH1106_status_t sh1106_status = SH1106_SUCCESS;
	SH1106_text_t sh1106_text;
	uint8_t idx = 0;
	// Navigation text parameters.
	sh1106_text.contrast = SH1106_TEXT_CONTRAST_NORMAL;
	sh1106_text.vertical_position = SH1106_TEXT_VERTICAL_POSITION_TOP;
	sh1106_text.flush_width_pixels = HMI_NAVIGATION_ZONE_WIDTH_PIXELS;
	// Navigation pages loop.
	for (idx=0 ; idx<HMI_DATA_PAGES_DISPLAYED ; idx++) {
		// Common
		sh1106_text.page = HMI_DATA_PAGE_ADDRESS[idx];
		// Left.
		sh1106_text.str = (char_t*) hmi_ctx.navigation_left[idx];
		sh1106_text.justification = STRING_JUSTIFICATION_LEFT;
		sh1106_status = SH1106_print_text(&sh1106_text);
		SH1106_status_check(HMI_ERROR_BASE_SH1106);
		// Right.
		sh1106_text.str = (char_t*) hmi_ctx.navigation_right[idx];
		sh1106_text.justification = STRING_JUSTIFICATION_RIGHT;
		sh1106_status = SH1106_print_text(&sh1106_text);
		SH1106_status_check(HMI_ERROR_BASE_SH1106);
	}
errors:
	return status;
}

/* PRINT DATA ZONE ON SCREEN.
 * @param:			None.
 * @return status:	Function executions status.
 */
static HMI_status_t _HMI_print_data(void) {
	// Local variables.
	HMI_status_t status = HMI_SUCCESS;
	SH1106_status_t sh1106_status = SH1106_SUCCESS;
	SH1106_text_t sh1106_text;
	uint8_t idx = 0;
	// Data text parameters.
	sh1106_text.justification = STRING_JUSTIFICATION_CENTER;
	sh1106_text.contrast = SH1106_TEXT_CONTRAST_NORMAL;
	sh1106_text.vertical_position = SH1106_TEXT_VERTICAL_POSITION_TOP;
	sh1106_text.flush_width_pixels = HMI_DATA_ZONE_WIDTH_PIXELS;
	// Data pages loop.
	for (idx=0 ; idx<HMI_DATA_PAGES_DISPLAYED ; idx++) {
		// Set page and string.
		sh1106_text.page = HMI_DATA_PAGE_ADDRESS[idx];
		sh1106_text.str = (char_t*) hmi_ctx.data[hmi_ctx.data_offset_index + idx];
		sh1106_status = SH1106_print_text(&sh1106_text);
		SH1106_status_check(HMI_ERROR_BASE_SH1106);
	}
errors:
	return status;
}

/* UPDATE TITLE ZONE.
 * @param screen:	View to display.
 * @return status:	Function executions status.
 */
static HMI_status_t _HMI_update_and_print_title(HMI_screen_t screen) {
	// Local variables.
	HMI_status_t status = HMI_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	NODE_status_t node_status = NODE_SUCCESS;
	char_t* text_ptr_1 = NULL;
	char_t* text_ptr_2 = NULL;
	// Reset text buffer.
	_HMI_text_flush();
	// Build title according to screen.
	switch (screen) {
	case HMI_SCREEN_NODES_LIST:
		status = STRING_append_string(hmi_ctx.text, HMI_DATA_ZONE_WIDTH_CHAR, (char_t*) HMI_TITLE_NODES_LIST, &hmi_ctx.text_width);
		STRING_status_check(HMI_ERROR_BASE_STRING);
		status = STRING_append_string(hmi_ctx.text, HMI_DATA_ZONE_WIDTH_CHAR, " [", &hmi_ctx.text_width);
		STRING_status_check(HMI_ERROR_BASE_STRING);
		status = STRING_append_value(hmi_ctx.text, HMI_DATA_ZONE_WIDTH_CHAR, NODES_LIST.count, STRING_FORMAT_DECIMAL, 0, &hmi_ctx.text_width);
		STRING_status_check(HMI_ERROR_BASE_STRING);
		status = STRING_append_string(hmi_ctx.text, HMI_DATA_ZONE_WIDTH_CHAR, "]", &hmi_ctx.text_width);
		STRING_status_check(HMI_ERROR_BASE_STRING);
		break;
	case HMI_SCREEN_NODES_SCAN:
		status = STRING_append_string(hmi_ctx.text, HMI_DATA_ZONE_WIDTH_CHAR, (char_t*) HMI_TITLE_NODES_LIST, &hmi_ctx.text_width);
		STRING_status_check(HMI_ERROR_BASE_STRING);
		status = STRING_append_string(hmi_ctx.text, HMI_DATA_ZONE_WIDTH_CHAR, "[-]", &hmi_ctx.text_width);
		STRING_status_check(HMI_ERROR_BASE_STRING);
		break;
	case HMI_SCREEN_NODE_DATA:
		node_status = NODE_get_name(&hmi_ctx.node, &text_ptr_1);
		switch (node_status) {
		case NODE_SUCCESS:
			text_ptr_2 = (text_ptr_1 != NULL) ? text_ptr_1 : (char_t*) HMI_TEXT_NA;
			status = STRING_append_string(hmi_ctx.text, HMI_DATA_ZONE_WIDTH_CHAR, text_ptr_2, &hmi_ctx.text_width);
			STRING_status_check(HMI_ERROR_BASE_STRING);
			break;
		case NODE_ERROR_NOT_SUPPORTED:
			status = STRING_append_string(hmi_ctx.text, HMI_DATA_ZONE_WIDTH_CHAR, (char_t*) HMI_TEXT_NA, &hmi_ctx.text_width);
			STRING_status_check(HMI_ERROR_BASE_STRING);
			break;
		default:
			NODE_status_check(HMI_ERROR_BASE_NODE);
			break;
		}
		status = STRING_append_string(hmi_ctx.text, HMI_DATA_ZONE_WIDTH_CHAR, " [", &hmi_ctx.text_width);
		STRING_status_check(HMI_ERROR_BASE_STRING);
#ifdef AM
		status = STRING_append_value(hmi_ctx.text, HMI_DATA_ZONE_WIDTH_CHAR, hmi_ctx.node.address, STRING_FORMAT_HEXADECIMAL, 1, &hmi_ctx.text_width);
#else
		status = STRING_append_string(hmi_ctx.text, HMI_DATA_ZONE_WIDTH_CHAR, (char_t*) HMI_TEXT_NA, &hmi_ctx.text_width);
#endif
		STRING_status_check(HMI_ERROR_BASE_STRING);
		status = STRING_append_string(hmi_ctx.text, HMI_DATA_ZONE_WIDTH_CHAR, "]", &hmi_ctx.text_width);
		STRING_status_check(HMI_ERROR_BASE_STRING);
		break;
	default:
		// Keep current title in all other cases.
		goto errors;
	}
	status = _HMI_print_title((char_t*) hmi_ctx.text);
errors:
	return status;
}

/* UPDATE NAVIGATION ZONE.
 * @param screen:	View to display.
 * @return status:	Function executions status.
 */
static HMI_status_t _HMI_update_and_print_navigation(HMI_screen_t screen) {
	// Local variables.
	HMI_status_t status = HMI_SUCCESS;
	uint8_t idx = 0;
	// Check screen.
	switch (screen) {
	case HMI_SCREEN_NODES_LIST:
	case HMI_SCREEN_NODE_DATA:
		for (idx=0 ; idx<HMI_DATA_PAGES_DISPLAYED ; idx++) {
			hmi_ctx.navigation_left[idx][0] = ((hmi_ctx.pointer_index == idx) && (hmi_ctx.data_depth != 0)) ? HMI_SYMBOL_SELECT : STRING_CHAR_SPACE;
			switch (idx) {
			case 0:
				hmi_ctx.navigation_right[idx][0] = (hmi_ctx.data_offset_index > 0) ? HMI_SYMBOL_TOP : STRING_CHAR_SPACE;
				break;
			case (HMI_DATA_PAGES_DISPLAYED - 1):
				hmi_ctx.navigation_right[idx][0] = (hmi_ctx.data_offset_index < (hmi_ctx.data_depth - HMI_DATA_PAGES_DISPLAYED)) ? HMI_SYMBOL_BOTTOM : STRING_CHAR_SPACE;
				break;
			default:
				hmi_ctx.navigation_right[idx][0] = STRING_CHAR_SPACE;
				break;
			}
		}
		break;
	default:
		// Disable navigation symbols in all other cases.
		for (idx=0 ; idx<HMI_DATA_PAGES_DISPLAYED ; idx++) {
			hmi_ctx.navigation_left[idx][0] = STRING_CHAR_SPACE;
			hmi_ctx.navigation_right[idx][0] = STRING_CHAR_SPACE;
		}
		break;
	}
	status = _HMI_print_navigation();
	return status;
}

/* UPDATE VALUE ON CURRENT STRING DATA INDEX.
 * @param:			None.
 * @return status:	Function execution status.
 */
static HMI_status_t _HMI_update_data(void) {
	// Local variables.
	HMI_status_t status = HMI_SUCCESS;
	NODE_status_t node_status = NODE_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	STRING_copy_t string_copy;
	char_t* text_ptr_1 = NULL;
	char_t* text_ptr_2 = NULL;
	// Common string copy parameter.
	string_copy.flush_char = STRING_CHAR_SPACE;
	string_copy.destination = (char_t*) hmi_ctx.data[hmi_ctx.data_index];
	string_copy.destination_size = HMI_DATA_ZONE_WIDTH_CHAR;
	// Check screen.
	if (hmi_ctx.screen != HMI_SCREEN_NODE_DATA) {
		status = HMI_ERROR_SCREEN;
		goto errors;
	}
	node_status = NODE_update_data(&hmi_ctx.node, hmi_ctx.data_index);
	// Check status.
	if (node_status != NODE_ERROR_NOT_SUPPORTED) {
		NODE_status_check(HMI_ERROR_BASE_NODE);
	}
	else {
		// Do not update data.
		goto errors;
	}
	// Read data.
	node_status = NODE_read_string_data(&hmi_ctx.node, hmi_ctx.data_index, &text_ptr_1, &text_ptr_2);
	// Check status and pointers.
	if (node_status != NODE_ERROR_NOT_SUPPORTED) {
		NODE_status_check(HMI_ERROR_BASE_NODE);
	}
	else {
		// Do not update data.
		goto errors;
	}
	// Update name.
	string_copy.source = text_ptr_1;
	string_copy.justification = STRING_JUSTIFICATION_LEFT;
	string_copy.flush_flag = 1;
	string_status = STRING_copy(&string_copy);
	STRING_status_check(HMI_ERROR_BASE_STRING);
	// Update value.
	string_copy.source = text_ptr_2;
	string_copy.justification = STRING_JUSTIFICATION_RIGHT;
	string_copy.flush_flag = 0;
	string_status = STRING_copy(&string_copy);
	STRING_status_check(HMI_ERROR_BASE_STRING);
errors:
	return status;
}

/* UPDATE DATA ZONE.
 * @param screen:	View to display.
 * @return status:	Function executions status.
 */
static HMI_status_t _HMI_update_all_data(HMI_screen_t screen) {
	// Local variables.
	HMI_status_t status = HMI_SUCCESS;
	NODE_status_t node_status = NODE_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	STRING_copy_t string_copy;
	char_t* text_ptr_1 = NULL;
	char_t* text_ptr_2 = NULL;
	uint8_t idx = 0;
	uint8_t last_string_data_index = 0;
	// Flush buffers.
	_HMI_data_flush();
	_HMI_text_flush();
	// Common parameters.
	string_copy.flush_char = STRING_CHAR_SPACE;
	string_copy.destination_size = HMI_DATA_ZONE_WIDTH_CHAR;
	// Build data according to screen.
	switch (screen) {
	case HMI_SCREEN_NODES_LIST:
		// Nodes loop.
		for (idx=0 ; idx<(NODES_LIST.count) ; idx++) {
			// Check size.
			if (idx >= HMI_DATA_PAGES_DEPTH) {
				status = HMI_ERROR_DATA_DEPTH_OVERFLOW;
				goto errors;
			}
			// Build node.
#ifdef AM
			hmi_ctx.node.address = NODES_LIST.list[idx].address;
#endif
			hmi_ctx.node.board_id = NODES_LIST.list[idx].board_id;
			// Get board name.
			node_status = NODE_get_name(&hmi_ctx.node, &text_ptr_1);
			switch (node_status) {
			case NODE_SUCCESS:
				string_copy.source = (text_ptr_1 != NULL) ? text_ptr_1 : (char_t*) HMI_TEXT_NA;
				break;
			case NODE_ERROR_NOT_SUPPORTED:
				string_copy.source = (char_t*) HMI_TEXT_NA;
				break;
			default:
				NODE_status_check(HMI_ERROR_BASE_NODE);
				break;
			}
			string_copy.destination = (char_t*) hmi_ctx.data[idx];
			string_copy.justification = STRING_JUSTIFICATION_LEFT;
			string_copy.flush_flag = 1;
			string_status = STRING_copy(&string_copy);
			STRING_status_check(HMI_ERROR_BASE_STRING);
			// Print node address.
			_HMI_text_flush();
#ifdef AM
			status = STRING_append_value(hmi_ctx.text, HMI_DATA_ZONE_WIDTH_CHAR, NODES_LIST.list[idx].address, STRING_FORMAT_HEXADECIMAL, 1, &hmi_ctx.text_width);
#else
			status = STRING_append_string(hmi_ctx.text, HMI_DATA_ZONE_WIDTH_CHAR, (char_t*) HMI_TEXT_NA, &hmi_ctx.text_width);
#endif
			STRING_status_check(HMI_ERROR_BASE_STRING);
			string_copy.source = (char_t*) hmi_ctx.text;
			string_copy.justification = STRING_JUSTIFICATION_RIGHT;
			string_copy.flush_flag = 0;
			string_status = STRING_copy(&string_copy);
			STRING_status_check(HMI_ERROR_BASE_STRING);
		}
		hmi_ctx.data_depth = NODES_LIST.count;
		break;
	case HMI_SCREEN_NODES_SCAN:
		// Common parameters.
		string_copy.justification = STRING_JUSTIFICATION_CENTER;
		string_copy.flush_flag = 1;
		// Lines loop.
		for (idx=0 ; idx<HMI_DATA_PAGES_DISPLAYED ; idx++) {
			string_copy.source = (char_t*) HMI_MESSAGE_NODES_SCAN_RUNNING[idx];
			string_copy.destination = (char_t*) hmi_ctx.data[idx];
			string_status = STRING_copy(&string_copy);
			STRING_status_check(HMI_ERROR_BASE_STRING);
		}
		break;
	case HMI_SCREEN_NODE_DATA:
		// Print temporary screen during data reading.
		string_copy.justification = STRING_JUSTIFICATION_CENTER;
		string_copy.flush_flag = 1;
		// Lines loop.
		for (idx=0 ; idx<HMI_DATA_PAGES_DISPLAYED ; idx++) {
			string_copy.source = (char_t*) HMI_MESSAGE_READING_DATA[idx];
			string_copy.destination = (char_t*) hmi_ctx.data[hmi_ctx.data_offset_index + idx];
			string_status = STRING_copy(&string_copy);
			STRING_status_check(HMI_ERROR_BASE_STRING);
		}
		status = _HMI_print_data();
		if (status != HMI_SUCCESS) goto errors;
		// Flush buffers.
		_HMI_data_flush();
		// Update all node data.
		node_status = NODE_update_all_data(&hmi_ctx.node);
		switch (node_status) {
		case NODE_SUCCESS:
			// Go to next step.
			break;
		case NODE_ERROR_NOT_SUPPORTED:
			// Unsupported node.
			string_copy.justification = STRING_JUSTIFICATION_CENTER;
			string_copy.flush_flag = 1;
			// Lines loop.
			for (idx=0 ; idx<HMI_DATA_PAGES_DISPLAYED ; idx++) {
				string_copy.source = (char_t*) HMI_MESSAGE_UNSUPPORTED_NODE[idx];
				string_copy.destination = (char_t*) hmi_ctx.data[idx];
				string_status = STRING_copy(&string_copy);
				STRING_status_check(HMI_ERROR_BASE_STRING);
			}
			goto errors;
			break;
		default:
			NODE_status_check(HMI_ERROR_BASE_NODE);
			break;
		}
		// Get number of lines.
		node_status = NODE_get_last_string_data_index(&hmi_ctx.node, &last_string_data_index);
		NODE_status_check(HMI_ERROR_BASE_NODE);
		// Check result.
		if (last_string_data_index == 0) {
			// No measurement returned.
			string_copy.justification = STRING_JUSTIFICATION_CENTER;
			string_copy.flush_flag = 1;
			// Lines loop.
			for (idx=0 ; idx<HMI_DATA_PAGES_DISPLAYED ; idx++) {
				string_copy.source = (char_t*) HMI_MESSAGE_NONE_MEASUREMENT[idx];
				string_copy.destination = (char_t*) hmi_ctx.data[idx];
				string_status = STRING_copy(&string_copy);
				STRING_status_check(HMI_ERROR_BASE_STRING);
			}
			goto errors;
		}
		// Data lines loop.
		for (idx=0 ; idx<last_string_data_index ; idx++) {
			// Check index.
			if (idx >= HMI_DATA_PAGES_DEPTH) {
				status = HMI_ERROR_DATA_DEPTH_OVERFLOW;
				goto errors;
			}
			// Read data.
			node_status = NODE_read_string_data(&hmi_ctx.node, idx, &text_ptr_1, &text_ptr_2);
			NODE_status_check(HMI_ERROR_BASE_NODE);
			// Update pointer.
			string_copy.destination = (char_t*) hmi_ctx.data[idx];
			// Print data name.
			string_copy.source = text_ptr_1;
			string_copy.justification = STRING_JUSTIFICATION_LEFT;
			string_copy.flush_flag = 1;
			string_status = STRING_copy(&string_copy);
			STRING_status_check(HMI_ERROR_BASE_STRING);
			// Print data value.
			string_copy.source = text_ptr_2;
			string_copy.justification = STRING_JUSTIFICATION_RIGHT;
			string_copy.flush_flag = 0;
			string_status = STRING_copy(&string_copy);
			STRING_status_check(HMI_ERROR_BASE_STRING);
		}
		// Update depth.
		hmi_ctx.data_depth = idx;
		break;
	case HMI_SCREEN_ERROR:
		// Common parameters.
		string_copy.justification = STRING_JUSTIFICATION_CENTER;
		string_copy.flush_flag = 1;
		// Line 1.
		string_copy.source = (char_t*) HMI_TEXT_ERROR;
		string_copy.destination = (char_t*) hmi_ctx.data[0];
		string_status = STRING_copy(&string_copy);
		STRING_status_check(HMI_ERROR_BASE_STRING);
		// Line 2.
		status = STRING_append_value(hmi_ctx.text, HMI_DATA_ZONE_WIDTH_CHAR, hmi_ctx.status, STRING_FORMAT_HEXADECIMAL, 1, &hmi_ctx.text_width);
		STRING_status_check(HMI_ERROR_BASE_STRING);
		string_copy.source = (char_t*) hmi_ctx.text;
		string_copy.destination = (char_t*) hmi_ctx.data[1];
		string_status = STRING_copy(&string_copy);
		STRING_status_check(HMI_ERROR_BASE_STRING);
		break;
	default:
		status = HMI_ERROR_SCREEN;
		goto errors;
	}
errors:
	return status;
}

/* RESET NAVIGATION TO THE TOP OF THE LIST.
 * @param:	None.
 * @return:	None.
 */
static void _HMI_reset_navigation(void) {
	// Reset indexes.
	hmi_ctx.data_index = 0;
	hmi_ctx.data_offset_index = 0;
	hmi_ctx.pointer_index = 0;
}

/* ENABLE HMI INTERRUPTS.
 * @param:	None.
 * @return:	None.
 */
static void _HMI_enable_irq(void) {
	// Clear flags.
	EXTI_clear_all_flags();
	hmi_ctx.irq_flags = 0;
	// Enable interrupts.
	NVIC_enable_interrupt(NVIC_INTERRUPT_EXTI_0_1);
	NVIC_enable_interrupt(NVIC_INTERRUPT_EXTI_2_3);
	NVIC_enable_interrupt(NVIC_INTERRUPT_EXTI_4_15);
}

/* DISABLE HMI INTERRUPTS.
 * @param:	None.
 * @return:	None.
 */
static void _HMI_disable_irq(void) {
	// Disable interrupts.
	NVIC_disable_interrupt(NVIC_INTERRUPT_EXTI_0_1);
	NVIC_disable_interrupt(NVIC_INTERRUPT_EXTI_2_3);
	NVIC_disable_interrupt(NVIC_INTERRUPT_EXTI_4_15);
}

/* UPDATE FULL DISPLAY.
 * @param screen:	Screen to display..
 * @return:			None.
 */
static HMI_status_t _HMI_update(HMI_screen_t screen, uint8_t update_all_data, uint8_t update_navigation) {
	// Local variables.
	HMI_status_t status = HMI_SUCCESS;
	// Disable interrupts during update.
	_HMI_disable_irq();
	// Check if screen has changed.
	if (hmi_ctx.screen != screen) {
		_HMI_reset_navigation();
		// Update  and print title.
		status = _HMI_update_and_print_title(screen);
		if (status != HMI_SUCCESS) goto errors;
	}
	// Update data (must be performed before navigation to have the correct depth value).
	if (update_all_data != 0) {
		status = _HMI_update_all_data(screen);
		if (status != HMI_SUCCESS) goto errors;
	}
	// Print data in all cases.
	status = _HMI_print_data();
	if (status != HMI_SUCCESS) goto errors;
	// Update navigation.
	if (update_navigation != 0) {
		status = _HMI_update_and_print_navigation(screen);
		if (status != HMI_SUCCESS) goto errors;
	}
	// Update context.
	hmi_ctx.screen = screen;
errors:
	_HMI_enable_irq();
	return status;
}

/* ENCODER SWITCH IRQ CALLBACK.
 * @param:	None.
 * @return:	None.
 */
static HMI_status_t _HMI_irq_callback_encoder_switch(void) {
	// Local variables.
	HMI_status_t status = HMI_SUCCESS;
	// Check current screen.
	switch (hmi_ctx.screen) {
	case HMI_SCREEN_NODES_LIST:
		// Update current node.
#ifdef AM
		hmi_ctx.node.address = NODES_LIST.list[hmi_ctx.data_index].address;
#endif
		hmi_ctx.node.board_id = NODES_LIST.list[hmi_ctx.data_index].board_id;
		// Update screen.
		status = _HMI_update(HMI_SCREEN_NODE_DATA, 1, 1);
		break;
	default:
		// Nothing to do in other views.
		break;
	}
	return status;
}

/* ENCODER FORWARD IRQ CALLBACK.
 * @param:	None.
 * @return:	None.
 */
static HMI_status_t _HMI_irq_callback_encoder_forward(void) {
	// Local variables.
	HMI_status_t status = HMI_SUCCESS;
	// Increment data select index.
	if (hmi_ctx.data_index < (hmi_ctx.data_depth - 1)) {
		hmi_ctx.data_index++;
	}
	if (hmi_ctx.pointer_index < (HMI_DATA_PAGES_DISPLAYED - 1)) {
		hmi_ctx.pointer_index++;
	}
	else {
		if ((hmi_ctx.data_depth - HMI_DATA_PAGES_DISPLAYED) > 0) {
			if (hmi_ctx.data_offset_index < (hmi_ctx.data_depth - HMI_DATA_PAGES_DISPLAYED)) {
				hmi_ctx.data_offset_index++;
			}
		}
	}
	// Update display.
	status = _HMI_update(hmi_ctx.screen, 0, 1);
	return status;
}

/* ENCODER BACKWARD IRQ CALLBACK.
 * @param:	None.
 * @return:	None.
 */
static HMI_status_t _HMI_irq_callback_encoder_backward(void) {
	// Local variables.
	HMI_status_t status = HMI_SUCCESS;
	// Increment data select index.
	if (hmi_ctx.data_index > 0) {
		hmi_ctx.data_index--;
	}
	if (hmi_ctx.pointer_index > 0) {
		hmi_ctx.pointer_index--;
	}
	else {
		if (hmi_ctx.data_offset_index > 0) {
			hmi_ctx.data_offset_index--;
		}
	}
	// Update display.
	status = _HMI_update(hmi_ctx.screen, 0, 1);
	return status;
}

/* COMMAND ON IRQ CALLBACK.
 * @param:	None.
 * @return:	None.
 */
static HMI_status_t _HMI_irq_callback_cmd_on(void) {
	// Local variables.
	HMI_status_t status = HMI_SUCCESS;
	NODE_status_t node_status = NODE_SUCCESS;
	NODE_access_status_t write_status;
	// Execute node register write function.
	node_status = NODE_write_string_data(&hmi_ctx.node, hmi_ctx.data_index, 1, &write_status);
	// Check status.
	if (node_status != NODE_ERROR_NOT_SUPPORTED) {
		NODE_status_check(HMI_ERROR_BASE_NODE);
	}
	// Update display is write operation succedded.
	if (write_status.all == 0) {
		// Read written data.
		status = _HMI_update_data();
		if (status != HMI_SUCCESS) goto errors;
		// Update display.
		status = _HMI_update(hmi_ctx.screen, 0, 0);
	}
errors:
	return status;
}

/* COMMAND OFF IRQ CALLBACK.
 * @param:	None.
 * @return:	None.
 */
static HMI_status_t _HMI_irq_callback_cmd_off(void) {
	// Local variables.
	HMI_status_t status = HMI_SUCCESS;
	NODE_status_t node_status = NODE_SUCCESS;
	NODE_access_status_t write_status;
	// Execute node register write function.
	node_status = NODE_write_string_data(&hmi_ctx.node, hmi_ctx.data_index, 0, &write_status);
	// Check status.
	if (node_status != NODE_ERROR_NOT_SUPPORTED) {
		NODE_status_check(HMI_ERROR_BASE_NODE);
	}
	// Update display is write operation succedded.
	if (write_status.all == 0) {
		// Read written data.
		status = _HMI_update_data();
		if (status != HMI_SUCCESS) goto errors;
		// Update display.
		status = _HMI_update(hmi_ctx.screen, 0, 0);
	}
errors:
	return status;
}

/* BP1 IRQ CALLBACK.
 * @param:	None.
 * @return:	None.
 */
static HMI_status_t _HMI_irq_callback_bp1(void) {
	// Local variables.
	HMI_status_t status = HMI_SUCCESS;
	NODE_status_t node_status = NODE_SUCCESS;
	// Update screen.
	status = _HMI_update(HMI_SCREEN_NODES_SCAN, 1, 1);
	if (status != HMI_SUCCESS) goto errors;
	// Disable HMI interrupts.
	_HMI_disable_irq();
	// Perform nodes scan.
	node_status = NODE_scan();
	NODE_status_check(HMI_ERROR_BASE_NODE);
	// Update screen.
	status = _HMI_update(HMI_SCREEN_NODES_LIST, 1, 1);
errors:
	// Enable HMI interrupts.
	_HMI_enable_irq();
	return status;
}

/* BP2 IRQ CALLBACK.
 * @param:	None.
 * @return:	None.
 */
static HMI_status_t _HMI_irq_callback_bp2(void) {
	// Local variables.
	HMI_status_t status = HMI_SUCCESS;
	// Update screen.
	_HMI_reset_navigation();
	status = _HMI_update(HMI_SCREEN_NODES_LIST, 1, 1);
	if (status != HMI_SUCCESS) goto errors;
errors:
	return status;
}

/* BP3 IRQ CALLBACK.
 * @param:	None.
 * @return:	None.
 */
static HMI_status_t _HMI_irq_callback_bp3(void) {
	// Local variables.
	HMI_status_t status = HMI_SUCCESS;
	// Update all data.
	status = _HMI_update(hmi_ctx.screen, 1, 0);
	return status;
}

/* HMI INTERNAL STATE MACHINE.
 * @param:			None.
 * @return status:	Function execution status.
 */
static HMI_status_t _HMI_state_machine(void) {
	// Local variables.
	uint8_t idx = 0;
	HMI_status_t status = HMI_SUCCESS;
	I2C_status_t i2c1_status = I2C_SUCCESS;
	LPTIM_status_t lptim1_status = LPTIM_SUCCESS;
	SH1106_status_t sh1106_status = SH1106_SUCCESS;
	// Read state.
	switch (hmi_ctx.state) {
	case HMI_STATE_INIT:
		// Turn HMI on.
		i2c1_status = I2C1_power_on();
		I2C1_status_check(HMI_ERROR_BASE_I2C);
		// Init OLED screen.
		sh1106_status = SH1106_init();
		SH1106_status_check(HMI_ERROR_BASE_SH1106);
		// Display DINFox logo.
		sh1106_status = SH1106_print_image(DINFOX_LOGO);
		SH1106_status_check(HMI_ERROR_BASE_SH1106);
		lptim1_status = LPTIM1_delay_milliseconds(1000, 1);
		LPTIM1_status_check(HMI_ERROR_BASE_LPTIM);
		SH1106_clear();
		// Enable external interrupts.
		_HMI_enable_irq();
		// Print nodes list.
		status = _HMI_update(HMI_SCREEN_NODES_LIST, 1, 1);
		if (status != HMI_SUCCESS) goto errors;
		// Compute next state.
		hmi_ctx.state = HMI_STATE_IDLE;
		break;
	case HMI_STATE_IDLE:
		// Process IRQ flags.
		for (idx=0 ; idx<HMI_IRQ_LAST ; idx++) {
			// Check flag.
			if ((hmi_ctx.irq_flags & (0b1 << idx)) != 0) {
				// Execute callback and clear flag.
				status = hmi_ctx.irq_callbacks[idx]();
				hmi_ctx.irq_flags &= ~(0b1 << idx);
				// Exit in case of error.
				if (status != HMI_SUCCESS) goto errors;
			}
		}
		break;
	case HMI_STATE_UNUSED:
		// Nothing to do.
		break;
	default:
		break;
	}
errors:
	return status;
}

/*** HMI functions ***/

/* INIT HMI INTERFACE.
 * @param:	None.
 * @return:	None.
 */
void HMI_init(void) {
	// Init context.
#ifdef AM
	hmi_ctx.node.address = 0xFF;
#endif
	hmi_ctx.node.board_id = DINFOX_BOARD_ID_ERROR;
	_HMI_reset_navigation();
	// Init callbacks.
	hmi_ctx.irq_callbacks[HMI_IRQ_ENCODER_SWITCH] = &_HMI_irq_callback_encoder_switch;
	hmi_ctx.irq_callbacks[HMI_IRQ_ENCODER_FORWARD] = &_HMI_irq_callback_encoder_forward;
	hmi_ctx.irq_callbacks[HMI_IRQ_ENCODER_BACKWARD] = &_HMI_irq_callback_encoder_backward;
	hmi_ctx.irq_callbacks[HMI_IRQ_CMD_ON] = &_HMI_irq_callback_cmd_on;
	hmi_ctx.irq_callbacks[HMI_IRQ_CMD_OFF] = &_HMI_irq_callback_cmd_off;
	hmi_ctx.irq_callbacks[HMI_IRQ_BP1] = &_HMI_irq_callback_bp1;
	hmi_ctx.irq_callbacks[HMI_IRQ_BP2] = &_HMI_irq_callback_bp2;
	hmi_ctx.irq_callbacks[HMI_IRQ_BP3] = &_HMI_irq_callback_bp3;
	// Init buffers ending.
	_HMI_data_flush();
	_HMI_text_flush();
	// Init buttons.
	GPIO_configure(&GPIO_BP1, GPIO_MODE_INPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	EXTI_configure_gpio(&GPIO_BP1, EXTI_TRIGGER_RISING_EDGE);
	GPIO_configure(&GPIO_BP2, GPIO_MODE_INPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	EXTI_configure_gpio(&GPIO_BP2, EXTI_TRIGGER_RISING_EDGE);
	GPIO_configure(&GPIO_BP3, GPIO_MODE_INPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	EXTI_configure_gpio(&GPIO_BP3, EXTI_TRIGGER_RISING_EDGE);
	// Init switch.
	GPIO_configure(&GPIO_CMD_ON, GPIO_MODE_INPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	EXTI_configure_gpio(&GPIO_CMD_ON, EXTI_TRIGGER_RISING_EDGE);
	GPIO_configure(&GPIO_CMD_OFF, GPIO_MODE_INPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	EXTI_configure_gpio(&GPIO_CMD_OFF, EXTI_TRIGGER_RISING_EDGE);
	// Init encoder.
	GPIO_configure(&GPIO_ENC_SW, GPIO_MODE_INPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	EXTI_configure_gpio(&GPIO_ENC_SW, EXTI_TRIGGER_RISING_EDGE);
	GPIO_configure(&GPIO_ENC_CHA, GPIO_MODE_INPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	EXTI_configure_gpio(&GPIO_ENC_CHA, EXTI_TRIGGER_RISING_EDGE);
	GPIO_configure(&GPIO_ENC_CHB, GPIO_MODE_INPUT, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	EXTI_configure_gpio(&GPIO_ENC_CHB, EXTI_TRIGGER_RISING_EDGE);
	// Set interrupts priority.
	NVIC_set_priority(NVIC_INTERRUPT_EXTI_0_1, 1);
	NVIC_set_priority(NVIC_INTERRUPT_EXTI_2_3, 1);
	NVIC_set_priority(NVIC_INTERRUPT_EXTI_4_15, 1);
}

/* MAIN TASK OF HMI.
 * @param:			None.
 * @return status:	Function execution status.
 */
HMI_status_t HMI_task(void) {
	// Local variables.
	HMI_status_t status = HMI_SUCCESS;
	LPUART_status_t lpuart1_status = LPUART_SUCCESS;
	LPTIM_status_t lptim1_status = LPTIM_SUCCESS;
	// Init context.
	hmi_ctx.screen = HMI_SCREEN_OFF;
	hmi_ctx.state = ((hmi_ctx.irq_flags & (0b1 << HMI_IRQ_ENCODER_SWITCH)) != 0) ? HMI_STATE_INIT : HMI_STATE_UNUSED;
	// Turn bus interface on.
	lpuart1_status = LPUART1_power_on();
	LPUART1_status_check(HMI_ERROR_BASE_LPUART);
	// Process HMI while it is used.
	while (hmi_ctx.state != HMI_STATE_UNUSED) {
		// Perform state machine.
		status = _HMI_state_machine();
		if (status != HMI_SUCCESS) {
			// Print error on screen.
			hmi_ctx.status = status;
			_HMI_update(HMI_SCREEN_ERROR, 1, 1);
			// Delay and exit.
			lptim1_status = LPTIM1_delay_milliseconds((HMI_UNUSED_DURATION_THRESHOLD_SECONDS * 1000), 1);
			LPTIM1_status_check(HMI_ERROR_BASE_LPTIM);
			goto errors;
		}
		// Start auto power-off timer.
		lptim1_status = LPTIM1_start(HMI_UNUSED_DURATION_THRESHOLD_SECONDS * 1000);
		LPTIM1_status_check(HMI_ERROR_BASE_LPTIM);
		// Enter stop mode.
		PWR_enter_stop_mode();
		// Wake-up.
		IWDG_reload();
		LPTIM1_stop();
		// Check LPTIM flag.
		if ((LPTIM1_get_wake_up_flag() != 0) && (hmi_ctx.irq_flags == 0)) {
			// Auto power-off.
			hmi_ctx.state = HMI_STATE_UNUSED;
		}
	}
errors:
	// Turn HMI off.
	LPTIM1_stop();
	I2C1_power_off();
	_HMI_disable_irq();
	// Turn bus interface off.
	LPUART1_power_off();
	return status;
}

/* SET IRQ FLAG (CALLED BY EXTI INTERRUPT).
 * @param irq_flag:	IRQ flag to set.
 * @return:			None.
 */
void HMI_set_irq_flag(HMI_irq_flag_t irq_flag) {
	// Set local flag.
	hmi_ctx.irq_flags |= (0b1 << irq_flag);
}
