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

#define HMI_SH1106_PAGE_TITLE					0
#define HMI_SH1106_PAGE_DATA_FIRST				3

#define HMI_WAKEUP_PERIOD_SECONDS				1
#define HMI_UNUSED_DURATION_THRESHOLD_SECONDS	5

#define HMI_STRING_VALUE_BUFFER_SIZE			16

static const char_t HMI_TEXT_NODES_LIST[] =	"NODES LIST";
static const char_t HMI_TEXT_NODE_LIST_EMPTY[] = "NODES LIST EMPTY";
static const char_t HMI_TEXT_PRESS[] = "PRESS";
static const char_t HMI_TEXT_NODES_SCAN_BUTTON[] = "NODES SCAN BUTTON";

/*** HMI local structures ***/

typedef void (*_HMI_irq_callback)(void);

typedef enum {
	HMI_STATE_INIT,
	HMI_STATE_IDLE,
	HMI_STATE_UNUSED,
	HMI_STATE_LAST,
} HMI_state_t;

typedef struct {
	HMI_state_t state;
	volatile uint32_t irq_flags;
	_HMI_irq_callback irq_callbacks[HMI_IRQ_LAST];
	uint8_t unused_duration_seconds;
	char_t text[SH1106_SCREEN_WIDTH_CHAR];
	uint8_t text_width;
	SH1106_text_t sh1106_text;
	SH1106_horizontal_line_t sh1106_line;
} HMI_context_t;

/*** HMI local global variables ***/

static HMI_context_t hmi_ctx;

/*** HMI local functions ***/

/* GENERIC MACRO TO ADD A CHARACTER TO THE REPLY BUFFER.
 * @param character:	Character to add.
 * @return:				None.
 */
#define _HMI_text_add_char(character) { \
	hmi_ctx.text[hmi_ctx.text_width] = character; \
	hmi_ctx.text_width = (hmi_ctx.text_width + 1) % SH1106_SCREEN_WIDTH_CHAR; \
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
	for (idx=0 ; idx<SH1106_SCREEN_WIDTH_CHAR ; idx++) hmi_ctx.text[idx] = STRING_CHAR_NULL;
	hmi_ctx.text_width = 0;
}

/* PRINT FIRST LINE TITLE ON SCREEN.
 * @param title:	String to print.
 * @return status:	Function executions status.
 */
static HMI_status_t _HMI_print_title(char_t* title) {
	// Local variables.
	HMI_status_t status = HMI_SUCCESS;
	SH1106_status_t sh1106_status = SH1106_SUCCESS;
	// Display node list menu.
	hmi_ctx.sh1106_text.str = title;
	hmi_ctx.sh1106_text.page = HMI_SH1106_PAGE_TITLE;
	hmi_ctx.sh1106_text.justification = SH1106_JUSTIFICATION_CENTER;
	hmi_ctx.sh1106_text.contrast = SH1106_TEXT_CONTRAST_INVERTED;
	hmi_ctx.sh1106_text.vertical_position = SH1106_TEXT_VERTICAL_POSITION_BOTTOM;
	hmi_ctx.sh1106_text.line_erase_flag = 1;
	// Add line.
	hmi_ctx.sh1106_line.line_pixels = 8;
	hmi_ctx.sh1106_line.width_pixels = SH1106_SCREEN_WIDTH_PIXELS;
	hmi_ctx.sh1106_line.justification = SH1106_JUSTIFICATION_CENTER;
	hmi_ctx.sh1106_line.contrast = SH1106_TEXT_CONTRAST_NORMAL;
	hmi_ctx.sh1106_line.line_erase_flag = 1;
	// Print title.
	sh1106_status = SH1106_print_text(&hmi_ctx.sh1106_text);
	SH1106_status_check(HMI_ERROR_BASE_SH1106);
	sh1106_status = SH1106_print_horizontal_line(&hmi_ctx.sh1106_line);
	SH1106_status_check(HMI_ERROR_BASE_SH1106);
errors:
	return status;
}

/* PRINT NODES LIST ON SCREEN.
 * @param:			None.
 * @return status:	Function executions status.
 */
static HMI_status_t _HMI_print_nodes_list(void) {
	// Local variables.
	HMI_status_t status = HMI_SUCCESS;
	SH1106_status_t sh1106_status = SH1106_SUCCESS;
	uint8_t idx = 0;
	// Display node list menu.
	_HMI_text_flush();
	_HMI_text_add_string((char_t*) HMI_TEXT_NODES_LIST);
	_HMI_text_add_string(" [");
	_HMI_text_add_value(rs485_common_ctx.nodes_count, STRING_FORMAT_DECIMAL, 0);
	_HMI_text_add_string("]");
	status = _HMI_print_title(hmi_ctx.text);
	if (status != HMI_SUCCESS) goto errors;
	// Set common text parameters.
	hmi_ctx.sh1106_text.str = (char_t*) hmi_ctx.text;
	hmi_ctx.sh1106_text.page = HMI_SH1106_PAGE_DATA_FIRST;
	hmi_ctx.sh1106_text.contrast = SH1106_TEXT_CONTRAST_NORMAL;
	hmi_ctx.sh1106_text.vertical_position = SH1106_TEXT_VERTICAL_POSITION_TOP;
	hmi_ctx.sh1106_text.line_erase_flag = 1;
	// Check nodes count.
	if (rs485_common_ctx.nodes_count == 0) {
		hmi_ctx.sh1106_text.justification = SH1106_JUSTIFICATION_CENTER;
		hmi_ctx.sh1106_text.str = (char_t*) HMI_TEXT_NODE_LIST_EMPTY;
		sh1106_status = SH1106_print_text(&hmi_ctx.sh1106_text);
		SH1106_status_check(HMI_ERROR_BASE_SH1106);
		hmi_ctx.sh1106_text.page += 2;
		hmi_ctx.sh1106_text.str = (char_t*) HMI_TEXT_PRESS;
		sh1106_status = SH1106_print_text(&hmi_ctx.sh1106_text);
		SH1106_status_check(HMI_ERROR_BASE_SH1106);
		hmi_ctx.sh1106_text.page += 2;
		hmi_ctx.sh1106_text.str = (char_t*) HMI_TEXT_NODES_SCAN_BUTTON;
		sh1106_status = SH1106_print_text(&hmi_ctx.sh1106_text);
		SH1106_status_check(HMI_ERROR_BASE_SH1106);
	}
	else {
		hmi_ctx.sh1106_text.justification = SH1106_JUSTIFICATION_LEFT;
		// Loop on nodes list.
		for (idx=0 ; idx<(rs485_common_ctx.nodes_count) ; idx++) {
			hmi_ctx.sh1106_text.str = (char_t*) DINFOX_BOARD_ID_NAME[rs485_common_ctx.nodes_list[idx].board_id];
			sh1106_status = SH1106_print_text(&hmi_ctx.sh1106_text);
			SH1106_status_check(HMI_ERROR_BASE_SH1106);
			hmi_ctx.sh1106_text.page += 2;
			// Exit when last page is reached.
			if (hmi_ctx.sh1106_text.page > SH1106_SCREEN_HEIGHT_LINE) break;
		}
	}
errors:
	return status;
}

/* ENCODER SWITCH IRQ CALLBACK.
 * @param:	None.
 * @return:	None.
 */
static void _HMI_irq_callback_encoder_switch(void) {
	LED_start_single_blink(LED_COLOR_WHITE, 10);
}

/* ENCODER FORWARD IRQ CALLBACK.
 * @param:	None.
 * @return:	None.
 */
static void _HMI_irq_callback_encoder_forward(void) {
	LED_start_single_blink(LED_COLOR_GREEN, 10);
}

/* ENCODER BACKWARD IRQ CALLBACK.
 * @param:	None.
 * @return:	None.
 */
static void _HMI_irq_callback_encoder_backward(void) {
	LED_start_single_blink(LED_COLOR_RED, 10);
}

/* COMMAND ON IRQ CALLBACK.
 * @param:	None.
 * @return:	None.
 */
static void _HMI_irq_callback_cmd_on(void) {
	LED_start_single_blink(LED_COLOR_YELLOW, 10);
}

/* COMMAND OFF IRQ CALLBACK.
 * @param:	None.
 * @return:	None.
 */
static void _HMI_irq_callback_cmd_off(void) {
	LED_start_single_blink(LED_COLOR_CYAN, 10);
}

/* BP1 IRQ CALLBACK.
 * @param:	None.
 * @return:	None.
 */
static void _HMI_irq_callback_bp1(void) {
	LED_start_single_blink(LED_COLOR_MAGENTA, 10);
}

/* BP2 IRQ CALLBACK.
 * @param:	None.
 * @return:	None.
 */
static void _HMI_irq_callback_bp2(void) {
	LED_start_single_blink(LED_COLOR_BLUE, 10);
}

/* BP3 IRQ CALLBACK.
 * @param:	None.
 * @return:	None.
 */
static void _HMI_irq_callback_bp3(void) {
	LED_start_single_blink(LED_COLOR_RED, 10);
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
		hmi_ctx.irq_flags = 0;
		EXTI_clear_all_flags();
		NVIC_enable_interrupt(NVIC_INTERRUPT_EXTI_2_3);
		NVIC_enable_interrupt(NVIC_INTERRUPT_EXTI_4_15);
		// Print nodes list.
		status = _HMI_print_nodes_list();
		// Compute next state.
		hmi_ctx.state = HMI_STATE_IDLE;
		break;
	case HMI_STATE_IDLE:
		// Process IRQ flags.
		for (idx=0 ; idx<HMI_IRQ_LAST ; idx++) {
			// Check flag.
			if ((hmi_ctx.irq_flags & (0b1 << idx)) != 0) {
				// Execute callback and clear flag.
				hmi_ctx.irq_callbacks[idx]();
				hmi_ctx.irq_flags &= ~(0b1 << idx);
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
	hmi_ctx.irq_callbacks[HMI_IRQ_ENCODER_SWITCH] = &_HMI_irq_callback_encoder_switch;
	hmi_ctx.irq_callbacks[HMI_IRQ_ENCODER_FORWARD] = &_HMI_irq_callback_encoder_forward;
	hmi_ctx.irq_callbacks[HMI_IRQ_ENCODER_BACKWARD] = &_HMI_irq_callback_encoder_backward;
	hmi_ctx.irq_callbacks[HMI_IRQ_CMD_ON] = &_HMI_irq_callback_cmd_on;
	hmi_ctx.irq_callbacks[HMI_IRQ_CMD_OFF] = &_HMI_irq_callback_cmd_off;
	hmi_ctx.irq_callbacks[HMI_IRQ_BP1] = &_HMI_irq_callback_bp1;
	hmi_ctx.irq_callbacks[HMI_IRQ_BP2] = &_HMI_irq_callback_bp2;
	hmi_ctx.irq_callbacks[HMI_IRQ_BP3] = &_HMI_irq_callback_bp3;
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
