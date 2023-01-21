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
#include "mapping.h"
#include "nvic.h"
#include "pwr.h"
#include "rs485_common.h"
#include "rtc.h"
#include "sh1106.h"
#include "string.h"
#include "types.h"

/*** HMI macros ***/

#define HMI_DATA_PAGES_DISPLAYED				3
#define HMI_DATA_PAGES_DEPTH					32

#define HMI_WAKEUP_PERIOD_SECONDS				1
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
static const char_t* HMI_MESSAGE_NODES_SCAN_RUNNING[HMI_DATA_PAGES_DISPLAYED] = {"NODES SCAN", "RUNNING", "..."};
static const char_t* DINFOX_BOARD_ID_NAME[DINFOX_BOARD_ID_LAST] = {"LVRM", "BPSM", "DDRM", "UHFM", "GPSM", "SM", "DIM", "RRM", "DMM", "MPMCM"};

/*** HMI local structures ***/

typedef HMI_status_t (*_HMI_irq_callback)(void);

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
	HMI_state_t state;
	HMI_screen_t screen;
	volatile uint8_t irq_flags;
	_HMI_irq_callback irq_callbacks[HMI_IRQ_LAST];
	uint8_t unused_duration_seconds;
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
} HMI_context_t;

/*** HMI local global variables ***/

static HMI_context_t hmi_ctx;
static const uint8_t HMI_DATA_PAGE_ADDRESS[HMI_DATA_PAGES_DISPLAYED] = {HMI_PAGE_ADDRESS_DATA_1, HMI_PAGE_ADDRESS_DATA_2, HMI_PAGE_ADDRESS_DATA_3};

/*** HMI local functions ***/

/* GENERIC MACRO TO ADD A CHARACTER TO THE REPLY BUFFER.
 * @param character:	Character to add.
 * @return:				None.
 */
#define _HMI_text_add_char(character) { \
	hmi_ctx.text[hmi_ctx.text_width] = character; \
	hmi_ctx.text_width = (hmi_ctx.text_width + 1) % HMI_DATA_ZONE_WIDTH_CHAR; \
}

/* APPEND A STRING TO THE REPONSE BUFFER.
 * @param tx_string:	String to add.
 * @return:				None.
 */
static void _HMI_text_add_string(char_t* tx_string) {
	// Fill TX buffer with new bytes.
	while (*tx_string) {
		_HMI_text_add_char(*(tx_string++));
	}
}

/* APPEND A VALUE TO THE REPONSE BUFFER.
 * @param tx_value:		Value to add.
 * @param format:       Printing format.
 * @param print_prefix: Print base prefix is non zero.
 * @return:				None.
 */
static void _HMI_text_add_value(int32_t tx_value, STRING_format_t format, uint8_t print_prefix) {
	// Local variables.
	STRING_status_t string_status = STRING_SUCCESS;
	char_t str_value[HMI_STRING_VALUE_BUFFER_SIZE];
	uint8_t idx = 0;
	// Reset string.
	for (idx=0 ; idx<HMI_STRING_VALUE_BUFFER_SIZE ; idx++) str_value[idx] = STRING_CHAR_NULL;
	// Convert value to string.
	string_status = STRING_value_to_string(tx_value, format, print_prefix, str_value);
	STRING_error_check();
	// Add string.
	_HMI_text_add_string(str_value);
}

/* FLUSH TEXT BUFFER.
 * @param:	None.
 * @return:	None.
 */
static void _HMI_text_flush(void) {
	// Local variables.
	uint8_t idx = 0;
	// Flush string.
	for (idx=0 ; idx<HMI_DATA_ZONE_WIDTH_CHAR ; idx++) hmi_ctx.text[idx] = STRING_CHAR_NULL;
	hmi_ctx.text_width = 0;
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
 * @param:			None.
 * @return status:	Function executions status.
 */
static HMI_status_t _HMI_update_title(void) {
	// Local variables.
	HMI_status_t status = HMI_SUCCESS;
	// Reset text buffer.
	_HMI_text_flush();
	// Build title according to screen.
	switch (hmi_ctx.screen) {
	case HMI_SCREEN_NODES_LIST:
		_HMI_text_add_string((char_t*) HMI_TITLE_NODES_LIST);
		_HMI_text_add_string(" [");
		_HMI_text_add_value(rs485_common_ctx.nodes_count, STRING_FORMAT_DECIMAL, 0);
		_HMI_text_add_string("]");
		break;
	case HMI_SCREEN_NODES_SCAN:
		_HMI_text_add_string((char_t*) HMI_TITLE_NODES_LIST);
		_HMI_text_add_string(" [-]");
		break;
	case HMI_SCREEN_NODE_DATA:
		// TODO.
		break;
	default:
		status = HMI_ERROR_SCREEN;
		goto errors;
	}
	status = _HMI_print_title((char_t*) hmi_ctx.text);
errors:
	return status;
}

static HMI_status_t _HMI_update_navigation(void) {
	// Local variables.
	HMI_status_t status = HMI_SUCCESS;
	uint8_t idx = 0;
	// Check screen.
	switch (hmi_ctx.screen) {
	case HMI_SCREEN_NODES_LIST:
	case HMI_SCREEN_NODE_DATA:
		for (idx=0 ; idx<HMI_DATA_PAGES_DISPLAYED ; idx++) {
			hmi_ctx.navigation_left[idx][0] = (hmi_ctx.pointer_index == idx) ? HMI_SYMBOL_SELECT : STRING_CHAR_SPACE;
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
		for (idx=0 ; idx<HMI_DATA_PAGES_DISPLAYED ; idx++) {
			hmi_ctx.navigation_left[idx][0] = STRING_CHAR_SPACE;
			hmi_ctx.navigation_right[idx][0] = STRING_CHAR_SPACE;
		}
		break;
	}
	status = _HMI_print_navigation();
	return status;
}

/* UPDATE DATA ZONE.
 * @param:			None.
 * @return status:	Function executions status.
 */
static HMI_status_t _HMI_update_data(void) {
	// Local variables.
	HMI_status_t status = HMI_SUCCESS;
	STRING_status_t string_status = STRING_SUCCESS;
	STRING_copy_t string_copy;
	// Common parameters.
	string_copy.flush_char = STRING_CHAR_SPACE;
	// Build title according to screen.
	switch (hmi_ctx.screen) {
	case HMI_SCREEN_NODES_LIST:
		// Common parameters.
		string_copy.destination_size = HMI_DATA_ZONE_WIDTH_CHAR;
		// Nodes loop.
		for (hmi_ctx.data_depth=0 ; hmi_ctx.data_depth<(rs485_common_ctx.nodes_count) ; hmi_ctx.data_depth++) {
			// Update pointer.
			string_copy.destination = (char_t*) hmi_ctx.data[hmi_ctx.data_depth];
			// Print board name.
			string_copy.source = (char_t*) DINFOX_BOARD_ID_NAME[rs485_common_ctx.nodes_list[hmi_ctx.data_depth].board_id];
			string_copy.justification = STRING_JUSTIFICATION_LEFT;
			string_copy.flush_flag = 1;
			string_status = STRING_copy(&string_copy);
			STRING_status_check(HMI_ERROR_BASE_STRING);
			// Print RS485 address.
			_HMI_text_flush();
			_HMI_text_add_value(rs485_common_ctx.nodes_list[hmi_ctx.data_depth].address, STRING_FORMAT_HEXADECIMAL, 1);
			string_copy.source = (char_t*) hmi_ctx.text;
			string_copy.justification = STRING_JUSTIFICATION_RIGHT;
			string_copy.flush_flag = 0;
			string_status = STRING_copy(&string_copy);
			STRING_status_check(HMI_ERROR_BASE_STRING);
		}
		break;
	case HMI_SCREEN_NODES_SCAN:
		// Common parameters.
		string_copy.destination_size = HMI_DATA_ZONE_WIDTH_CHAR;
		string_copy.justification = STRING_JUSTIFICATION_CENTER;
		string_copy.flush_flag = 1;
		// Lines loop.
		for (hmi_ctx.data_depth=0 ; hmi_ctx.data_depth<HMI_DATA_PAGES_DISPLAYED ; hmi_ctx.data_depth++) {
			string_copy.source = (char_t*) HMI_MESSAGE_NODES_SCAN_RUNNING[hmi_ctx.data_depth];
			string_copy.destination = (char_t*) hmi_ctx.data[hmi_ctx.data_depth];
			string_status = STRING_copy(&string_copy);
			STRING_status_check(HMI_ERROR_BASE_STRING);
		}
		break;
	case HMI_SCREEN_NODE_DATA:
		// TODO.
		break;
	default:
		status = HMI_ERROR_SCREEN;
		goto errors;
	}
	status = _HMI_print_data();
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

/* UPDATE FULL DISPLAY.
 * @param screen:	Screen to display..
 * @return:			None.
 */
static HMI_status_t _HMI_update(HMI_screen_t screen) {
	// Local variables.
	HMI_status_t status = HMI_SUCCESS;
	// Update title zone if required.
	if (hmi_ctx.screen != screen) {
		// Update context.
		hmi_ctx.screen = screen;
		_HMI_reset_navigation();
		// Update all zones if required.
		status = _HMI_update_title();
		if (status != HMI_SUCCESS) goto errors;
	}
	// Update navigation and data zones.
	// Note: data update must be performed before navigation to have correct depth value.
	status = _HMI_update_data();
	if (status != HMI_SUCCESS) goto errors;
	status = _HMI_update_navigation();

errors:
	return status;
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

/* ENCODER SWITCH IRQ CALLBACK.
 * @param:	None.
 * @return:	None.
 */
static HMI_status_t _HMI_irq_callback_encoder_switch(void) {
	// Local variables.
	HMI_status_t status = HMI_SUCCESS;
	// TODO
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
	status = _HMI_update(hmi_ctx.screen);
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
	status = _HMI_update(hmi_ctx.screen);
	return status;
}

/* COMMAND ON IRQ CALLBACK.
 * @param:	None.
 * @return:	None.
 */
static HMI_status_t _HMI_irq_callback_cmd_on(void) {
	// Local variables.
	HMI_status_t status = HMI_SUCCESS;
	// TODO
	return status;
}

/* COMMAND OFF IRQ CALLBACK.
 * @param:	None.
 * @return:	None.
 */
static HMI_status_t _HMI_irq_callback_cmd_off(void) {
	// Local variables.
	HMI_status_t status = HMI_SUCCESS;
	// TODO
	return status;
}

/* BP1 IRQ CALLBACK.
 * @param:	None.
 * @return:	None.
 */
static HMI_status_t _HMI_irq_callback_bp1(void) {
	// Local variables.
	HMI_status_t status = HMI_SUCCESS;
	LPUART_status_t lpuart1_status = LPUART_SUCCESS;
	RS485_status_t rs485_status = RS485_SUCCESS;
	// Update screen.
	status = _HMI_update(HMI_SCREEN_NODES_SCAN);
	if (status != HMI_SUCCESS) goto errors;
	// Disable HMI interrupts.
	_HMI_disable_irq();
	// Perform nodes scan.
	lpuart1_status = LPUART1_power_on();
	LPUART1_status_check(HMI_ERROR_BASE_LPUART);
	rs485_status = RS485_scan_nodes();
	RS485_status_check(HMI_ERROR_BASE_RS485);
	// Update screen.
	status = _HMI_update(HMI_SCREEN_NODES_LIST);
errors:
	// Enable HMI interrupts and turn RS485 interface off.
	_HMI_enable_irq();
	LPUART1_power_off();
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
	status = _HMI_update(HMI_SCREEN_NODES_LIST);
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
		status = _HMI_update(HMI_SCREEN_NODES_LIST);
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
	// Local variables.
	uint8_t idx = 0;
	// Init context.
	hmi_ctx.irq_callbacks[HMI_IRQ_ENCODER_SWITCH] = &_HMI_irq_callback_encoder_switch;
	hmi_ctx.irq_callbacks[HMI_IRQ_ENCODER_FORWARD] = &_HMI_irq_callback_encoder_forward;
	hmi_ctx.irq_callbacks[HMI_IRQ_ENCODER_BACKWARD] = &_HMI_irq_callback_encoder_backward;
	hmi_ctx.irq_callbacks[HMI_IRQ_CMD_ON] = &_HMI_irq_callback_cmd_on;
	hmi_ctx.irq_callbacks[HMI_IRQ_CMD_OFF] = &_HMI_irq_callback_cmd_off;
	hmi_ctx.irq_callbacks[HMI_IRQ_BP1] = &_HMI_irq_callback_bp1;
	hmi_ctx.irq_callbacks[HMI_IRQ_BP2] = &_HMI_irq_callback_bp2;
	hmi_ctx.irq_callbacks[HMI_IRQ_BP3] = &_HMI_irq_callback_bp3;
	// Init buffers ending.
	for (idx=0 ; idx<HMI_DATA_PAGES_DEPTH ; idx++) {
		hmi_ctx.data[idx][HMI_DATA_ZONE_WIDTH_CHAR] = STRING_CHAR_NULL;
	}
	for (idx=0 ; idx<HMI_DATA_PAGES_DISPLAYED ; idx++) {
		hmi_ctx.navigation_left[idx][HMI_NAVIGATION_ZONE_WIDTH_CHAR] = STRING_CHAR_NULL;
		hmi_ctx.navigation_right[idx][HMI_NAVIGATION_ZONE_WIDTH_CHAR] = STRING_CHAR_NULL;
	}
	hmi_ctx.text[HMI_DATA_ZONE_WIDTH_CHAR] = STRING_CHAR_NULL;
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
	NVIC_set_priority(NVIC_INTERRUPT_EXTI_0_1, 0);
	NVIC_set_priority(NVIC_INTERRUPT_EXTI_2_3, 1);
	NVIC_set_priority(NVIC_INTERRUPT_EXTI_4_15, 2);
}

/* MAIN TASK OF HMI.
 * @param:			None.
 * @return status:	Function execution status.
 */
HMI_status_t HMI_task(void) {
	// Local variables.
	HMI_status_t status = HMI_SUCCESS;
	RTC_status_t rtc_status = RTC_SUCCESS;
	// Init context.
	hmi_ctx.screen = HMI_SCREEN_OFF;
	hmi_ctx.state = ((hmi_ctx.irq_flags & (0b1 << HMI_IRQ_ENCODER_SWITCH)) != 0) ? HMI_STATE_INIT : HMI_STATE_UNUSED;
	hmi_ctx.unused_duration_seconds = 0;
	// Start periodic wakeup timer.
	RTC_clear_wakeup_timer_flag();
	rtc_status = RTC_start_wakeup_timer(HMI_WAKEUP_PERIOD_SECONDS);
	RTC_status_check(HMI_ERROR_BASE_RTC);
	// Process HMI while it is used.
	while (hmi_ctx.state != HMI_STATE_UNUSED) {
		// Perform state machine.
		status = _HMI_state_machine();
		if (status != HMI_SUCCESS) goto errors;
		// Enter sleep mode.
		PWR_enter_sleep_mode();
		// Wake-up.
		IWDG_reload();
		// Check RTC flag.
		if (RTC_get_wakeup_timer_flag() != 0) {
			// Clear flag.
			RTC_clear_wakeup_timer_flag();
			// Manage unused duration;
			if (hmi_ctx.irq_flags == 0) {
				hmi_ctx.unused_duration_seconds += HMI_WAKEUP_PERIOD_SECONDS;
			}
		}
		else {
			hmi_ctx.unused_duration_seconds = 0;
		}
		// Auto power off.
		if (hmi_ctx.unused_duration_seconds >= HMI_UNUSED_DURATION_THRESHOLD_SECONDS) {
			hmi_ctx.state = HMI_STATE_UNUSED;
		}
	}
errors:
	// Turn HMI off.
	I2C1_power_off();
	_HMI_disable_irq();
	// Stop periodic wakeup timer.
	RTC_stop_wakeup_timer();
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
