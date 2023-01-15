/*
 * hmi.c
 *
 *  Created on: 14 jan. 2023
 *      Author: Ludo
 */

#include "hmi.h"

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
#include "types.h"

/*** HMI macros ***/

#define HMI_SH1106_PAGE_TITLE					0
#define HMI_SH1106_PAGE_DATA_FIRST				3

#define HMI_WAKEUP_PERIOD_SECONDS				1
#define HMI_UNUSED_DURATION_THRESHOLD_SECONDS	5

static const char_t HMI_TEXT_NODES_LIST[] =	"NODES LIST";
static const char_t HMI_TEXT_NODE_LIST_EMPTY[] = "NODES LIST EMPTY";
static const char_t HMI_TEXT_PRESS[] = "PRESS";
static const char_t HMI_TEXT_NODES_SCAN_BUTTON[] = "NODES SCAN BUTTON";

/*** HMI local structures ***/

typedef enum {
	HMI_STATE_INIT,
	HMI_STATE_NODE_LIST,
	HMI_STATE_NODE_DATA,
	HMI_STATE_UNUSED,
	HMI_STATE_LAST,
} HMI_state_t;

typedef struct {
	HMI_state_t state;
	volatile uint8_t irq_flags;
	uint8_t unused_duration_seconds;
	SH1106_text_t text;
	SH1106_horizontal_line_t line;
} HMI_context_t;

/*** HMI local global variables ***/

static HMI_context_t hmi_ctx;

/*** HMI local functions ***/

/* PRINT FIRST LINE TITLE ON SCREEN.
 * @param title:	String to print.
 * @return status:	Function executions status.
 */
static HMI_status_t _HMI_print_title(const char_t* title) {
	// Local variables.
	HMI_status_t status = HMI_SUCCESS;
	SH1106_status_t sh1106_status = SH1106_SUCCESS;
	// Display node list menu.
	hmi_ctx.text.str = (char_t*) title;
	hmi_ctx.text.page = HMI_SH1106_PAGE_TITLE;
	hmi_ctx.text.justification = SH1106_JUSTIFICATION_CENTER;
	hmi_ctx.text.contrast = SH1106_TEXT_CONTRAST_INVERTED;
	hmi_ctx.text.vertical_position = SH1106_TEXT_VERTICAL_POSITION_BOTTOM;
	hmi_ctx.text.line_erase_flag = 1;
	// Add line.
	hmi_ctx.line.line_pixels = 8;
	hmi_ctx.line.width_pixels = SH1106_SCREEN_WIDTH_PIXELS;
	hmi_ctx.line.justification = SH1106_JUSTIFICATION_CENTER;
	hmi_ctx.line.contrast = SH1106_TEXT_CONTRAST_NORMAL;
	hmi_ctx.line.line_erase_flag = 1;
	// Print title.
	sh1106_status = SH1106_print_text(&hmi_ctx.text);
	SH1106_status_check(HMI_ERROR_BASE_SH1106);
	sh1106_status = SH1106_print_horizontal_line(&hmi_ctx.line);
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
	// Display node list menu.
	status = _HMI_print_title(HMI_TEXT_NODES_LIST);
	if (status != HMI_SUCCESS) goto errors;
	// Set common text parameters.
	hmi_ctx.text.page = HMI_SH1106_PAGE_DATA_FIRST;
	hmi_ctx.text.contrast = SH1106_TEXT_CONTRAST_NORMAL;
	hmi_ctx.text.vertical_position = SH1106_TEXT_VERTICAL_POSITION_TOP;
	hmi_ctx.text.line_erase_flag = 1;
	// Check nodes count.
	if (rs485_common_ctx.nodes_count == 0) {
		hmi_ctx.text.justification = SH1106_JUSTIFICATION_CENTER;
		hmi_ctx.text.str = (char_t*) HMI_TEXT_NODE_LIST_EMPTY;
		sh1106_status = SH1106_print_text(&hmi_ctx.text);
		SH1106_status_check(HMI_ERROR_BASE_SH1106);
		hmi_ctx.text.page += 2;
		hmi_ctx.text.str = (char_t*) HMI_TEXT_PRESS;
		sh1106_status = SH1106_print_text(&hmi_ctx.text);
		SH1106_status_check(HMI_ERROR_BASE_SH1106);
		hmi_ctx.text.page += 2;
		hmi_ctx.text.str = (char_t*) HMI_TEXT_NODES_SCAN_BUTTON;
		sh1106_status = SH1106_print_text(&hmi_ctx.text);
		SH1106_status_check(HMI_ERROR_BASE_SH1106);
	}
	else {
		// Loop on nodes list.

	}
errors:
	return status;
}

/* HMI INTERNAL STATE MACHINE.
 * @param:			None.
 * @return status:	Function execution status.
 */
static HMI_status_t _HMI_state_machine(void) {
	// Local variables.
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
		hmi_ctx.state = HMI_STATE_NODE_LIST;
		break;
	case HMI_STATE_NODE_LIST:
		if ((hmi_ctx.irq_flags & HMI_IRQ_MASK_BP1) != 0) {
			LED_start_single_blink(100, LED_COLOR_RED);
			hmi_ctx.irq_flags &= (~HMI_IRQ_MASK_BP1);
		}
		if ((hmi_ctx.irq_flags & HMI_IRQ_MASK_BP2) != 0) {
			LED_start_single_blink(100, LED_COLOR_GREEN);
			hmi_ctx.irq_flags &= (~HMI_IRQ_MASK_BP2);
		}
		if ((hmi_ctx.irq_flags & HMI_IRQ_MASK_BP3) != 0) {
			LED_start_single_blink(100, LED_COLOR_BLUE);
			hmi_ctx.irq_flags &= (~HMI_IRQ_MASK_BP3);
		}
		if ((hmi_ctx.irq_flags & HMI_IRQ_MASK_CMD_ON) != 0) {
			LED_start_single_blink(100, LED_COLOR_YELLOW);
			hmi_ctx.irq_flags &= (~HMI_IRQ_MASK_CMD_ON);
		}
		if ((hmi_ctx.irq_flags & HMI_IRQ_MASK_CMD_OFF) != 0) {
			LED_start_single_blink(100, LED_COLOR_CYAN);
			hmi_ctx.irq_flags &= (~HMI_IRQ_MASK_CMD_OFF);
		}
		if ((hmi_ctx.irq_flags & HMI_IRQ_MASK_ENCODER_FORWARD) != 0) {
			LED_start_single_blink(100, LED_COLOR_RED);
			hmi_ctx.irq_flags &= (~HMI_IRQ_MASK_ENCODER_FORWARD);
		}
		if ((hmi_ctx.irq_flags & HMI_IRQ_MASK_ENCODER_BACKWARD) != 0) {
			LED_start_single_blink(100, LED_COLOR_GREEN);
			hmi_ctx.irq_flags &= (~HMI_IRQ_MASK_ENCODER_BACKWARD);
		}
		break;
	case HMI_STATE_NODE_DATA:
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
	hmi_ctx.state = ((hmi_ctx.irq_flags & HMI_IRQ_MASK_ENCODER_SWITCH) != 0) ? HMI_STATE_INIT : HMI_STATE_UNUSED;
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
 * @param irq_mask:	IRQ mask to set.
 * @return:			None.
 */
void HMI_set_irq_flag(HMI_irq_mask_t irq_mask) {
	// Apply mask.
	hmi_ctx.irq_flags |= irq_mask;
}
