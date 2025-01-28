/*
 * hmi.c
 *
 *  Created on: 14 jan. 2023
 *      Author: Ludo
 */

#include "hmi.h"

#include "error.h"
#include "exti.h"
#include "gpio.h"
#include "gpio_mapping.h"
#include "hmi_node.h"
#include "i2c.h"
#include "i2c_address.h"
#include "iwdg.h"
#include "led.h"
#include "logo.h"
#include "math.h"
#include "node.h"
#include "nvic_priority.h"
#include "power.h"
#include "pwr.h"
#include "sh1106.h"
#include "sh1106_font.h"
#include "string.h"
#include "tim.h"
#include "types.h"
#include "una.h"

/*** HMI macros ***/

#define HMI_TIMER_INSTANCE                  TIM_INSTANCE_TIM2

#define HMI_DATA_PAGES_DISPLAYED            3
#define HMI_DATA_PAGES_DEPTH                32

#define HMI_UNUSED_DURATION_THRESHOLD_MS    5000

#define HMI_STRING_VALUE_BUFFER_SIZE        16

#define HMI_NAVIGATION_ZONE_WIDTH_CHAR      1
#define HMI_NAVIGATION_ZONE_WIDTH_PIXELS    (HMI_NAVIGATION_ZONE_WIDTH_CHAR * SH1106_FONT_CHAR_WIDTH_PIXELS)

#define HMI_DATA_ZONE_WIDTH_CHAR            (SH1106_SCREEN_WIDTH_CHAR - (2 * (HMI_NAVIGATION_ZONE_WIDTH_CHAR + 1)))
#define HMI_DATA_ZONE_WIDTH_PIXELS          (HMI_DATA_ZONE_WIDTH_CHAR * SH1106_FONT_CHAR_WIDTH_PIXELS)

#define HMI_SYMBOL_SELECT                   '>'
#define HMI_SYMBOL_TOP                      '\''
#define HMI_SYMBOL_BOTTOM                   '`'

/*** HMI local structures ***/

/*******************************************************************/
typedef HMI_status_t (*HMI_process_function_t)(void);

/*******************************************************************/
typedef enum {
    HMI_STATE_INIT = 0,
    HMI_STATE_IDLE,
    HMI_STATE_UNUSED,
    HMI_STATE_LAST,
} HMI_state_t;

/*******************************************************************/
typedef enum {
    HMI_SCREEN_OFF = 0,
    HMI_SCREEN_NODE_LIST,
    HMI_SCREEN_NODES_SCAN,
    HMI_SCREEN_NODE_DATA,
    HMI_SCREEN_ERROR,
    HMI_SCREEN_LAST,
} HMI_screen_t;

/*******************************************************************/
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

/*******************************************************************/
typedef enum {
    HMI_IRQ_ENCODER_SWITCH = 0,
    HMI_IRQ_ENCODER_FORWARD,
    HMI_IRQ_ENCODER_BACKWARD,
    HMI_IRQ_CMD_ON,
    HMI_IRQ_CMD_OFF,
    HMI_IRQ_BP1,
    HMI_IRQ_BP2,
    HMI_IRQ_BP3,
    HMI_IRQ_LAST
} HMI_irq_flag_t;

/*******************************************************************/
typedef struct {
    // Global.
    HMI_status_t status;
    HMI_state_t state;
    HMI_screen_t screen;
    volatile uint8_t irq_flags;
    volatile uint8_t auto_power_off_timer_flag;
    // Screen.
    char_t text[HMI_DATA_ZONE_WIDTH_CHAR + 1];
    uint32_t text_width;
    char_t data[HMI_DATA_PAGES_DEPTH][HMI_DATA_ZONE_WIDTH_CHAR + 1];
    uint8_t data_depth;
    uint8_t data_index;
    uint8_t data_offset_index;
    uint8_t pointer_index;
    char_t navigation_left[HMI_DATA_PAGES_DISPLAYED][HMI_NAVIGATION_ZONE_WIDTH_CHAR + 1];
    char_t navigation_right[HMI_DATA_PAGES_DISPLAYED][HMI_NAVIGATION_ZONE_WIDTH_CHAR + 1];
    SH1106_horizontal_line_t sh1106_line;
    // Current node.
    UNA_node_t node;
} HMI_context_t;

/*** HMI local functions declaration ***/

static HMI_status_t _HMI_process_encoder_switch(void);
static HMI_status_t _HMI_process_encoder_forward(void);
static HMI_status_t _HMI_process_encoder_backward(void);
static HMI_status_t _HMI_process_cmd_on(void);
static HMI_status_t _HMI_process_cmd_off(void);
static HMI_status_t _HMI_process_bp1(void);
static HMI_status_t _HMI_process_bp2(void);
static HMI_status_t _HMI_process_bp3(void);

/*** HMI local global variables ***/

static const HMI_process_function_t HMI_PROCESS_FUNCTION[HMI_IRQ_LAST] = {
    &_HMI_process_encoder_switch,
    &_HMI_process_encoder_forward,
    &_HMI_process_encoder_backward,
    &_HMI_process_cmd_on,
    &_HMI_process_cmd_off,
    &_HMI_process_bp1,
    &_HMI_process_bp2,
    &_HMI_process_bp3
};

static const char_t* HMI_TITLE_NODE_LIST = "NODES LIST";

static const char_t* HMI_TEXT_ERROR = "ERROR";
static const char_t* HMI_TEXT_NA = "N/A";

static const char_t* HMI_MESSAGE_NODES_SCAN_RUNNING[HMI_DATA_PAGES_DISPLAYED] = {"NODES SCAN", "RUNNING", "..." };
static const char_t* HMI_MESSAGE_UNSUPPORTED_NODE[HMI_DATA_PAGES_DISPLAYED] = { "UNSUPPORTED", "NODE", STRING_NULL };
static const char_t* HMI_MESSAGE_NONE_MEASUREMENT[HMI_DATA_PAGES_DISPLAYED] = { "NONE", "MEASUREMENT", "ON THIS NODE" };
static const char_t* HMI_MESSAGE_READING_DATA[HMI_DATA_PAGES_DISPLAYED] = { "READING DATA", "...", STRING_NULL };

static const uint8_t HMI_DATA_PAGE_ADDRESS[HMI_DATA_PAGES_DISPLAYED] = { HMI_PAGE_ADDRESS_DATA_1, HMI_PAGE_ADDRESS_DATA_2, HMI_PAGE_ADDRESS_DATA_3 };

static HMI_context_t hmi_ctx;

/*** HMI local functions ***/

/*******************************************************************/
static void _HMI_text_flush(void) {
    // Local variables.
    uint8_t idx = 0;
    // Flush string.
    for (idx = 0; idx < (HMI_DATA_ZONE_WIDTH_CHAR + 1); idx++)
        hmi_ctx.text[idx] = STRING_CHAR_NULL;
    hmi_ctx.text_width = 0;
}

/*******************************************************************/
static void _HMI_data_flush(void) {
    // Local variables.
    uint8_t line_idx = 0;
    uint8_t char_idx = 0;
    // Line loop.
    for (line_idx = 0; line_idx < HMI_DATA_PAGES_DEPTH; line_idx++) {
        // Char loop.
        for (char_idx = 0; char_idx < (HMI_DATA_ZONE_WIDTH_CHAR + 1); char_idx++) {
            hmi_ctx.data[line_idx][char_idx] = STRING_CHAR_NULL;
        }
    }
    hmi_ctx.data_depth = 0;
}

/*******************************************************************/
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
    sh1106_status = SH1106_print_text(I2C_ADDRESS_SH1106_HMI, &sh1106_text);
    SH1106_exit_error(HMI_ERROR_BASE_SH1106);
    sh1106_status = SH1106_print_horizontal_line(I2C_ADDRESS_SH1106_HMI, &hmi_ctx.sh1106_line);
    SH1106_exit_error(HMI_ERROR_BASE_SH1106);
errors:
    return status;
}

/*******************************************************************/
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
    for (idx = 0; idx < HMI_DATA_PAGES_DISPLAYED; idx++) {
        // Common
        sh1106_text.page = HMI_DATA_PAGE_ADDRESS[idx];
        // Left.
        sh1106_text.str = (char_t*) hmi_ctx.navigation_left[idx];
        sh1106_text.justification = STRING_JUSTIFICATION_LEFT;
        sh1106_status = SH1106_print_text(I2C_ADDRESS_SH1106_HMI, &sh1106_text);
        SH1106_exit_error(HMI_ERROR_BASE_SH1106);
        // Right.
        sh1106_text.str = (char_t*) hmi_ctx.navigation_right[idx];
        sh1106_text.justification = STRING_JUSTIFICATION_RIGHT;
        sh1106_status = SH1106_print_text(I2C_ADDRESS_SH1106_HMI, &sh1106_text);
        SH1106_exit_error(HMI_ERROR_BASE_SH1106);
    }
errors:
    return status;
}

/*******************************************************************/
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
    for (idx = 0; idx < HMI_DATA_PAGES_DISPLAYED; idx++) {
        // Set page and string.
        sh1106_text.page = HMI_DATA_PAGE_ADDRESS[idx];
        sh1106_text.str = (char_t*) hmi_ctx.data[hmi_ctx.data_offset_index + idx];
        sh1106_status = SH1106_print_text(I2C_ADDRESS_SH1106_HMI, &sh1106_text);
        SH1106_exit_error(HMI_ERROR_BASE_SH1106);
    }
errors:
    return status;
}

/*******************************************************************/
static HMI_status_t _HMI_update_and_print_title(HMI_screen_t screen) {
    // Local variables.
    HMI_status_t status = HMI_SUCCESS;
    STRING_status_t string_status = STRING_SUCCESS;
    char_t* text_ptr = NULL;
    // Reset text buffer.
    _HMI_text_flush();
    // Build title according to screen.
    switch (screen) {
    case HMI_SCREEN_NODE_LIST:
        string_status = STRING_append_string(hmi_ctx.text, HMI_DATA_ZONE_WIDTH_CHAR, (char_t*) HMI_TITLE_NODE_LIST, &hmi_ctx.text_width);
        STRING_exit_error(HMI_ERROR_BASE_STRING);
        string_status = STRING_append_string(hmi_ctx.text, HMI_DATA_ZONE_WIDTH_CHAR, " [", &hmi_ctx.text_width);
        STRING_exit_error(HMI_ERROR_BASE_STRING);
        string_status = STRING_append_integer(hmi_ctx.text, HMI_DATA_ZONE_WIDTH_CHAR, NODE_LIST.count, STRING_FORMAT_DECIMAL, 0, &hmi_ctx.text_width);
        STRING_exit_error(HMI_ERROR_BASE_STRING);
        string_status = STRING_append_string(hmi_ctx.text, HMI_DATA_ZONE_WIDTH_CHAR, "]", &hmi_ctx.text_width);
        STRING_exit_error(HMI_ERROR_BASE_STRING);
        break;
    case HMI_SCREEN_NODES_SCAN:
        string_status = STRING_append_string(hmi_ctx.text, HMI_DATA_ZONE_WIDTH_CHAR, (char_t*) HMI_TITLE_NODE_LIST, &hmi_ctx.text_width);
        STRING_exit_error(HMI_ERROR_BASE_STRING);
        string_status = STRING_append_string(hmi_ctx.text, HMI_DATA_ZONE_WIDTH_CHAR, "[-]", &hmi_ctx.text_width);
        STRING_exit_error(HMI_ERROR_BASE_STRING);
        break;
    case HMI_SCREEN_NODE_DATA:
        text_ptr = ((hmi_ctx.node.board_id) < UNA_BOARD_ID_LAST) ? (char_t*) UNA_BOARD_NAME[hmi_ctx.node.board_id] : (char_t*) HMI_TEXT_NA;
        string_status = STRING_append_string(hmi_ctx.text, HMI_DATA_ZONE_WIDTH_CHAR, text_ptr, &hmi_ctx.text_width);
        STRING_exit_error(HMI_ERROR_BASE_STRING);
        string_status = STRING_append_string(hmi_ctx.text, HMI_DATA_ZONE_WIDTH_CHAR, " [", &hmi_ctx.text_width);
        STRING_exit_error(HMI_ERROR_BASE_STRING);
        string_status = STRING_append_integer(hmi_ctx.text, HMI_DATA_ZONE_WIDTH_CHAR, hmi_ctx.node.address, STRING_FORMAT_HEXADECIMAL, 1, &hmi_ctx.text_width);
        STRING_exit_error(HMI_ERROR_BASE_STRING);
        string_status = STRING_append_string(hmi_ctx.text, HMI_DATA_ZONE_WIDTH_CHAR, "]", &hmi_ctx.text_width);
        STRING_exit_error(HMI_ERROR_BASE_STRING);
        break;
    default:
        // Keep current title in all other cases.
        goto errors;
    }
    status = _HMI_print_title((char_t*) hmi_ctx.text);
errors:
    return status;
}

/*******************************************************************/
static HMI_status_t _HMI_update_and_print_navigation(HMI_screen_t screen) {
    // Local variables.
    HMI_status_t status = HMI_SUCCESS;
    uint8_t idx = 0;
    // Check screen.
    switch (screen) {
    case HMI_SCREEN_NODE_LIST:
    case HMI_SCREEN_NODE_DATA:
        for (idx = 0; idx < HMI_DATA_PAGES_DISPLAYED; idx++) {
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
        for (idx = 0; idx < HMI_DATA_PAGES_DISPLAYED; idx++) {
            hmi_ctx.navigation_left[idx][0] = STRING_CHAR_SPACE;
            hmi_ctx.navigation_right[idx][0] = STRING_CHAR_SPACE;
        }
        break;
    }
    status = _HMI_print_navigation();
    return status;
}

/*******************************************************************/
static HMI_status_t _HMI_update_data(void) {
    // Local variables.
    HMI_status_t status = HMI_SUCCESS;
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
    status = HMI_NODE_read_line(&hmi_ctx.node, hmi_ctx.data_index);
    if (status != HMI_SUCCESS) goto errors;
    // Read data.
    status = HMI_NODE_get_line_data(&hmi_ctx.node, hmi_ctx.data_index, &text_ptr_1, &text_ptr_2);
    if (status != HMI_SUCCESS) goto errors;
    // Update name.
    string_copy.source = text_ptr_1;
    string_copy.justification = STRING_JUSTIFICATION_LEFT;
    string_copy.flush_flag = 1;
    string_status = STRING_copy(&string_copy);
    STRING_exit_error(HMI_ERROR_BASE_STRING);
    // Update value.
    string_copy.source = text_ptr_2;
    string_copy.justification = STRING_JUSTIFICATION_RIGHT;
    string_copy.flush_flag = 0;
    string_status = STRING_copy(&string_copy);
    STRING_exit_error(HMI_ERROR_BASE_STRING);
errors:
    return status;
}

/*******************************************************************/
static HMI_status_t _HMI_update_all_data(HMI_screen_t screen) {
    // Local variables.
    HMI_status_t status = HMI_SUCCESS;
    STRING_status_t string_status = STRING_SUCCESS;
    STRING_copy_t string_copy;
    char_t* text_ptr_1 = NULL;
    char_t* text_ptr_2 = NULL;
    uint8_t idx = 0;
    uint8_t last_line_index = 0;
    // Flush buffers.
    _HMI_data_flush();
    _HMI_text_flush();
    // Common parameters.
    string_copy.flush_char = STRING_CHAR_SPACE;
    string_copy.destination_size = HMI_DATA_ZONE_WIDTH_CHAR;
    // Build data according to screen.
    switch (screen) {
    case HMI_SCREEN_NODE_LIST:
        // Nodes loop.
        for (idx = 0; idx < (NODE_LIST.count); idx++) {
            // Check size.
            if (idx >= HMI_DATA_PAGES_DEPTH) {
                status = HMI_ERROR_DATA_DEPTH_OVERFLOW;
                goto errors;
            }
            // Build node.
            hmi_ctx.node.address = NODE_LIST.list[idx].address;
            hmi_ctx.node.board_id = NODE_LIST.list[idx].board_id;
            // Get board name.
            string_copy.source = ((hmi_ctx.node.board_id) < UNA_BOARD_ID_LAST) ? (char_t*) UNA_BOARD_NAME[hmi_ctx.node.board_id] : (char_t*) HMI_TEXT_NA;
            string_copy.destination = (char_t*) hmi_ctx.data[idx];
            string_copy.justification = STRING_JUSTIFICATION_LEFT;
            string_copy.flush_flag = 1;
            string_status = STRING_copy(&string_copy);
            STRING_exit_error(HMI_ERROR_BASE_STRING);
            // Print node address.
            _HMI_text_flush();
            string_status = STRING_append_integer(hmi_ctx.text, HMI_DATA_ZONE_WIDTH_CHAR, NODE_LIST.list[idx].address, STRING_FORMAT_HEXADECIMAL, 1, &hmi_ctx.text_width);
            STRING_exit_error(HMI_ERROR_BASE_STRING);
            string_copy.source = (char_t*) hmi_ctx.text;
            string_copy.justification = STRING_JUSTIFICATION_RIGHT;
            string_copy.flush_flag = 0;
            string_status = STRING_copy(&string_copy);
            STRING_exit_error(HMI_ERROR_BASE_STRING);
        }
        hmi_ctx.data_depth = NODE_LIST.count;
        break;
    case HMI_SCREEN_NODES_SCAN:
        // Common parameters.
        string_copy.justification = STRING_JUSTIFICATION_CENTER;
        string_copy.flush_flag = 1;
        // Lines loop.
        for (idx = 0; idx < HMI_DATA_PAGES_DISPLAYED; idx++) {
            string_copy.source = (char_t*) HMI_MESSAGE_NODES_SCAN_RUNNING[idx];
            string_copy.destination = (char_t*) hmi_ctx.data[idx];
            string_status = STRING_copy(&string_copy);
            STRING_exit_error(HMI_ERROR_BASE_STRING);
        }
        break;
    case HMI_SCREEN_NODE_DATA:
        // Print temporary screen during data reading.
        string_copy.justification = STRING_JUSTIFICATION_CENTER;
        string_copy.flush_flag = 1;
        // Lines loop.
        for (idx = 0; idx < HMI_DATA_PAGES_DISPLAYED; idx++) {
            string_copy.source = (char_t*) HMI_MESSAGE_READING_DATA[idx];
            string_copy.destination = (char_t*) hmi_ctx.data[hmi_ctx.data_offset_index + idx];
            string_status = STRING_copy(&string_copy);
            STRING_exit_error(HMI_ERROR_BASE_STRING);
        }
        status = _HMI_print_data();
        if (status != HMI_SUCCESS) goto errors;
        // Flush buffers.
        _HMI_data_flush();
        // Update all node data.
        status = HMI_NODE_read_line_all(&hmi_ctx.node);
        // Check status.
        if (status == (HMI_ERROR_BASE_NODE + NODE_ERROR_NOT_SUPPORTED)) {
            // Unsupported node.
            string_copy.justification = STRING_JUSTIFICATION_CENTER;
            string_copy.flush_flag = 1;
            // Lines loop.
            for (idx = 0; idx < HMI_DATA_PAGES_DISPLAYED; idx++) {
                string_copy.source = (char_t*) HMI_MESSAGE_UNSUPPORTED_NODE[idx];
                string_copy.destination = (char_t*) hmi_ctx.data[idx];
                string_status = STRING_copy(&string_copy);
                STRING_exit_error(HMI_ERROR_BASE_STRING);
            }
            goto errors;
        }
        else {
            if (status != HMI_SUCCESS) goto errors;
        }
        // Get number of lines.
        status = HMI_NODE_get_last_line_index(&hmi_ctx.node, &last_line_index);
        if (status != HMI_SUCCESS) goto errors;
        // Check result.
        if (last_line_index == 0) {
            // No measurement returned.
            string_copy.justification = STRING_JUSTIFICATION_CENTER;
            string_copy.flush_flag = 1;
            // Lines loop.
            for (idx = 0; idx < HMI_DATA_PAGES_DISPLAYED; idx++) {
                string_copy.source = (char_t*) HMI_MESSAGE_NONE_MEASUREMENT[idx];
                string_copy.destination = (char_t*) hmi_ctx.data[idx];
                string_status = STRING_copy(&string_copy);
                STRING_exit_error(HMI_ERROR_BASE_STRING);
            }
            goto errors;
        }
        // Data lines loop.
        for (idx = 0; idx < last_line_index; idx++) {
            // Check index.
            if (idx >= HMI_DATA_PAGES_DEPTH) {
                status = HMI_ERROR_DATA_DEPTH_OVERFLOW;
                goto errors;
            }
            // Read data.
            status = HMI_NODE_get_line_data(&hmi_ctx.node, idx, &text_ptr_1, &text_ptr_2);
            if (status != HMI_SUCCESS) goto errors;
            // Update pointer.
            string_copy.destination = (char_t*) hmi_ctx.data[idx];
            // Print data name.
            string_copy.source = text_ptr_1;
            string_copy.justification = STRING_JUSTIFICATION_LEFT;
            string_copy.flush_flag = 1;
            string_status = STRING_copy(&string_copy);
            STRING_exit_error(HMI_ERROR_BASE_STRING);
            // Print data value.
            string_copy.source = text_ptr_2;
            string_copy.justification = STRING_JUSTIFICATION_RIGHT;
            string_copy.flush_flag = 0;
            string_status = STRING_copy(&string_copy);
            STRING_exit_error(HMI_ERROR_BASE_STRING);
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
        STRING_exit_error(HMI_ERROR_BASE_STRING);
        // Line 2.
        string_status = STRING_append_integer(hmi_ctx.text, HMI_DATA_ZONE_WIDTH_CHAR, hmi_ctx.status, STRING_FORMAT_HEXADECIMAL, 1, &hmi_ctx.text_width);
        STRING_exit_error(HMI_ERROR_BASE_STRING);
        string_copy.source = (char_t*) hmi_ctx.text;
        string_copy.destination = (char_t*) hmi_ctx.data[1];
        string_status = STRING_copy(&string_copy);
        STRING_exit_error(HMI_ERROR_BASE_STRING);
        break;
    default:
        status = HMI_ERROR_SCREEN;
        goto errors;
    }
errors:
    return status;
}

/*******************************************************************/
static void _HMI_reset_navigation(void) {
    // Reset indexes.
    hmi_ctx.data_index = 0;
    hmi_ctx.data_offset_index = 0;
    hmi_ctx.pointer_index = 0;
}

/*******************************************************************/
static void _HMI_enable_irq(void) {
    // Clear flags.
    hmi_ctx.irq_flags = 0;
    // Enable interrupts.
    EXTI_enable_gpio_interrupt(&GPIO_BP1);
    EXTI_enable_gpio_interrupt(&GPIO_BP2);
    EXTI_enable_gpio_interrupt(&GPIO_BP3);
    EXTI_enable_gpio_interrupt(&GPIO_CMD_ON);
    EXTI_enable_gpio_interrupt(&GPIO_CMD_OFF);
    EXTI_enable_gpio_interrupt(&GPIO_ENC_CHA);
    EXTI_enable_gpio_interrupt(&GPIO_ENC_CHB);
}

/*******************************************************************/
static void _HMI_disable_irq(void) {
    // Disable interrupts.
    EXTI_disable_gpio_interrupt(&GPIO_BP1);
    EXTI_disable_gpio_interrupt(&GPIO_BP2);
    EXTI_disable_gpio_interrupt(&GPIO_BP3);
    EXTI_disable_gpio_interrupt(&GPIO_CMD_ON);
    EXTI_disable_gpio_interrupt(&GPIO_CMD_OFF);
    EXTI_disable_gpio_interrupt(&GPIO_ENC_CHA);
    EXTI_disable_gpio_interrupt(&GPIO_ENC_CHB);
}

/*******************************************************************/
static HMI_status_t _HMI_update(HMI_screen_t screen, uint8_t update_all_data, uint8_t update_navigation) {
    // Local variables.
    HMI_status_t status = HMI_SUCCESS;
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
    return status;
}

/*******************************************************************/
static void _HMI_irq_callback_encoder_switch(void) {
    // Set local flag.
    hmi_ctx.irq_flags |= (0b1 << HMI_IRQ_ENCODER_SWITCH);
}

/*******************************************************************/
static void _HMI_irq_callback_encoder_forward(void) {
    // Check channel B state.
    if (GPIO_read(&GPIO_ENC_CHB) == 0) {
        // Set local flag.
        hmi_ctx.irq_flags |= (0b1 << HMI_IRQ_ENCODER_FORWARD);
    }
}

/*******************************************************************/
static void _HMI_irq_callback_encoder_backward(void) {
    // Check channel B state.
    if (GPIO_read(&GPIO_ENC_CHA) == 0) {
        // Set local flag.
        hmi_ctx.irq_flags |= (0b1 << HMI_IRQ_ENCODER_BACKWARD);
    }
}

/*******************************************************************/
static void _HMI_irq_callback_cmd_on(void) {
    // Set local flag.
    hmi_ctx.irq_flags |= (0b1 << HMI_IRQ_CMD_ON);
}

/*******************************************************************/
static void _HMI_irq_callback_cmd_off(void) {
    // Set local flag.
    hmi_ctx.irq_flags |= (0b1 << HMI_IRQ_CMD_OFF);
}

/*******************************************************************/
static void _HMI_irq_callback_bp1(void) {
    // Set local flag.
    hmi_ctx.irq_flags |= (0b1 << HMI_IRQ_BP1);
}

/*******************************************************************/
static void _HMI_irq_callback_bp2(void) {
    // Set local flag.
    hmi_ctx.irq_flags |= (0b1 << HMI_IRQ_BP2);
}

/*******************************************************************/
static void _HMI_irq_callback_bp3(void) {
    // Set local flag.
    hmi_ctx.irq_flags |= (0b1 << HMI_IRQ_BP3);
}

/*******************************************************************/
static void _HMI_irq_callback_auto_power_off_timer(void) {
    // Set local flag.
    hmi_ctx.auto_power_off_timer_flag = 1;
}

/*******************************************************************/
static HMI_status_t _HMI_process_encoder_switch(void) {
    // Local variables.
    HMI_status_t status = HMI_SUCCESS;
    // Check current screen.
    switch (hmi_ctx.screen) {
    case HMI_SCREEN_NODE_LIST:
        // Update current node.
        hmi_ctx.node.address = NODE_LIST.list[hmi_ctx.data_index].address;
        hmi_ctx.node.board_id = NODE_LIST.list[hmi_ctx.data_index].board_id;
        // Update screen.
        status = _HMI_update(HMI_SCREEN_NODE_DATA, 1, 1);
        break;
    default:
        // Nothing to do in other views.
        break;
    }
    return status;
}

/*******************************************************************/
static HMI_status_t _HMI_process_encoder_forward(void) {
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

/*******************************************************************/
static HMI_status_t _HMI_process_encoder_backward(void) {
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

/*******************************************************************/
static HMI_status_t _HMI_process_cmd_on(void) {
    // Local variables.
    HMI_status_t status = HMI_SUCCESS;
    // Execute node register write function.
    status = HMI_NODE_write_line(&hmi_ctx.node, hmi_ctx.data_index, 1);
    if (status != HMI_SUCCESS) goto errors;
    // Read written data.
    status = _HMI_update_data();
    if (status != HMI_SUCCESS) goto errors;
    // Update display.
    status = _HMI_update(hmi_ctx.screen, 0, 0);
errors:
    return status;
}

/*******************************************************************/
static HMI_status_t _HMI_process_cmd_off(void) {
    // Local variables.
    HMI_status_t status = HMI_SUCCESS;
    // Execute node register write function.
    status = HMI_NODE_write_line(&hmi_ctx.node, hmi_ctx.data_index, 0);
    if (status != HMI_SUCCESS) goto errors;
    // Read written data.
    status = _HMI_update_data();
    if (status != HMI_SUCCESS) goto errors;
    // Update display.
    status = _HMI_update(hmi_ctx.screen, 0, 0);
errors:
    return status;
}

/*******************************************************************/
static HMI_status_t _HMI_process_bp1(void) {
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
    NODE_exit_error(HMI_ERROR_BASE_NODE);
    // Update screen.
    status = _HMI_update(HMI_SCREEN_NODE_LIST, 1, 1);
errors:
    // Enable HMI interrupts.
    _HMI_enable_irq();
    return status;
}

/*******************************************************************/
static HMI_status_t _HMI_process_bp2(void) {
    // Local variables.
    HMI_status_t status = HMI_SUCCESS;
    // Update screen.
    _HMI_reset_navigation();
    status = _HMI_update(HMI_SCREEN_NODE_LIST, 1, 1);
    if (status != HMI_SUCCESS) goto errors;
errors:
    return status;
}

/*******************************************************************/
static HMI_status_t _HMI_process_bp3(void) {
    // Local variables.
    HMI_status_t status = HMI_SUCCESS;
    // Update all data.
    status = _HMI_update(hmi_ctx.screen, 1, 0);
    return status;
}

/*******************************************************************/
static HMI_status_t _HMI_state_machine(void) {
    // Local variables.
    uint8_t idx = 0;
    HMI_status_t status = HMI_SUCCESS;
    LPTIM_status_t lptim_status = LPTIM_SUCCESS;
    SH1106_status_t sh1106_status = SH1106_SUCCESS;
    // Read state.
    switch (hmi_ctx.state) {
    case HMI_STATE_INIT:
        // Setup OLED screen.
        sh1106_status = SH1106_setup(I2C_ADDRESS_SH1106_HMI);
        SH1106_exit_error(HMI_ERROR_BASE_SH1106);
        // Display DINFox logo.
        sh1106_status = SH1106_print_image(I2C_ADDRESS_SH1106_HMI, DINFOX_LOGO);
        SH1106_exit_error(HMI_ERROR_BASE_SH1106);
        lptim_status = LPTIM_delay_milliseconds(1000, LPTIM_DELAY_MODE_STOP);
        LPTIM_exit_error(HMI_ERROR_BASE_LPTIM);
        SH1106_clear(I2C_ADDRESS_SH1106_HMI);
        // Enable external interrupts.
        _HMI_enable_irq();
        // Print nodes list.
        status = _HMI_update(HMI_SCREEN_NODE_LIST, 1, 1);
        if (status != HMI_SUCCESS) goto errors;
        // Compute next state.
        hmi_ctx.state = HMI_STATE_IDLE;
        break;
    case HMI_STATE_IDLE:
        // Process IRQ flags.
        for (idx = 0; idx < HMI_IRQ_LAST; idx++) {
            // Check flag.
            if ((hmi_ctx.irq_flags & (0b1 << idx)) != 0) {
                // Execute callback and clear flag.
                status = HMI_PROCESS_FUNCTION[idx]();
                hmi_ctx.irq_flags &= (uint8_t) (~(0b1 << idx));
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

/*******************************************************************/
HMI_status_t HMI_init_por(void) {
    // Local variables.
    HMI_status_t status = HMI_SUCCESS;
    // Init context.
    hmi_ctx.irq_flags = 0;
    hmi_ctx.auto_power_off_timer_flag = 0;
    // Init encoder switch used as wake-up signal.
    EXTI_configure_gpio(&GPIO_ENC_SW, GPIO_PULL_NONE, EXTI_TRIGGER_RISING_EDGE, &_HMI_irq_callback_encoder_switch, NVIC_PRIORITY_HMI_WAKE_UP);
    EXTI_enable_gpio_interrupt(&GPIO_ENC_SW);
    return status;
}

/*******************************************************************/
HMI_status_t HMI_init(void) {
    // Local variables.
    HMI_status_t status = HMI_SUCCESS;
    SH1106_status_t sh1106_status = SH1106_SUCCESS;
    TIM_status_t tim_status = TIM_SUCCESS;
    // Init context.
    hmi_ctx.node.address = 0xFF;
    hmi_ctx.node.board_id = UNA_BOARD_ID_ERROR;
    _HMI_reset_navigation();
    // Init buffers ending.
    _HMI_data_flush();
    _HMI_text_flush();
    // Init OLED screen.
    sh1106_status = SH1106_init();
    SH1106_exit_error(HMI_ERROR_BASE_SH1106);
    // Init auto-power off timer.
    tim_status = TIM_STD_init(HMI_TIMER_INSTANCE, NVIC_PRIORITY_HMI_TIMER);
    TIM_exit_error(HMI_ERROR_BASE_TIM);
    // Init buttons.
    EXTI_configure_gpio(&GPIO_BP1, GPIO_PULL_NONE, EXTI_TRIGGER_RISING_EDGE, &_HMI_irq_callback_bp1, NVIC_PRIORITY_HMI_INPUTS);
    EXTI_configure_gpio(&GPIO_BP2, GPIO_PULL_NONE, EXTI_TRIGGER_RISING_EDGE, &_HMI_irq_callback_bp2, NVIC_PRIORITY_HMI_INPUTS);
    EXTI_configure_gpio(&GPIO_BP3, GPIO_PULL_NONE, EXTI_TRIGGER_RISING_EDGE, &_HMI_irq_callback_bp3, NVIC_PRIORITY_HMI_INPUTS);
    // Init switch.
    EXTI_configure_gpio(&GPIO_CMD_ON, GPIO_PULL_NONE, EXTI_TRIGGER_RISING_EDGE, &_HMI_irq_callback_cmd_on, NVIC_PRIORITY_HMI_INPUTS);
    EXTI_configure_gpio(&GPIO_CMD_OFF, GPIO_PULL_NONE, EXTI_TRIGGER_RISING_EDGE, &_HMI_irq_callback_cmd_off, NVIC_PRIORITY_HMI_INPUTS);
    // Init encoder.
    EXTI_configure_gpio(&GPIO_ENC_CHA, GPIO_PULL_NONE, EXTI_TRIGGER_RISING_EDGE, &_HMI_irq_callback_encoder_forward, NVIC_PRIORITY_HMI_INPUTS);
    EXTI_configure_gpio(&GPIO_ENC_CHB, GPIO_PULL_NONE, EXTI_TRIGGER_RISING_EDGE, &_HMI_irq_callback_encoder_backward, NVIC_PRIORITY_HMI_INPUTS);
errors:
    return status;
}

/*******************************************************************/
HMI_status_t HMI_de_init(void) {
    // Local variables.
    HMI_status_t status = HMI_SUCCESS;
    SH1106_status_t sh1106_status = SH1106_SUCCESS;
    TIM_status_t tim_status = TIM_SUCCESS;
    // Release EXTI inputs.
    EXTI_release_gpio(&GPIO_BP1, GPIO_MODE_ANALOG);
    EXTI_release_gpio(&GPIO_BP2, GPIO_MODE_ANALOG);
    EXTI_release_gpio(&GPIO_BP3, GPIO_MODE_ANALOG);
    EXTI_release_gpio(&GPIO_CMD_ON, GPIO_MODE_ANALOG);
    EXTI_release_gpio(&GPIO_CMD_OFF, GPIO_MODE_ANALOG);
    EXTI_release_gpio(&GPIO_ENC_CHA, GPIO_MODE_ANALOG);
    EXTI_release_gpio(&GPIO_ENC_CHB, GPIO_MODE_ANALOG);
    // Release timer.
    tim_status = TIM_STD_de_init(HMI_TIMER_INSTANCE);
    TIM_exit_error(HMI_ERROR_BASE_TIM);
    // Release OLED screen.
    sh1106_status = SH1106_de_init();
    SH1106_exit_error(HMI_ERROR_BASE_SH1106);
errors:
    return status;
}

/*******************************************************************/
HMI_status_t HMI_process(void) {
    // Local variables.
    HMI_status_t status = HMI_SUCCESS;
    TIM_status_t tim_status = TIM_SUCCESS;
    // Check flag.
    if ((hmi_ctx.irq_flags & (0b1 << HMI_IRQ_ENCODER_SWITCH)) == 0) goto end;
    // Init context.
    hmi_ctx.screen = HMI_SCREEN_OFF;
    hmi_ctx.state = HMI_STATE_INIT;
    // Turn bus interface and HMI on.
    POWER_enable(POWER_REQUESTER_ID_HMI, POWER_DOMAIN_RS485, LPTIM_DELAY_MODE_STOP);
    POWER_enable(POWER_REQUESTER_ID_HMI, POWER_DOMAIN_HMI, LPTIM_DELAY_MODE_STOP);
    // Process HMI while it is used.
    while (hmi_ctx.state != HMI_STATE_UNUSED) {
        // Perform state machine.
        status = _HMI_state_machine();
        if (status != HMI_SUCCESS) goto errors;
        // Start auto power-off timer.
        tim_status = TIM_STD_start(HMI_TIMER_INSTANCE, HMI_UNUSED_DURATION_THRESHOLD_MS, TIM_UNIT_MS, &_HMI_irq_callback_auto_power_off_timer);
        TIM_exit_error(HMI_ERROR_BASE_TIM);
        // Enter stop mode.
        PWR_enter_sleep_mode();
        // Wake-up.
        IWDG_reload();
        // Check flags.
        if ((hmi_ctx.auto_power_off_timer_flag != 0) && (hmi_ctx.irq_flags == 0)) {
            // Clear flag.
            hmi_ctx.auto_power_off_timer_flag = 0;
            // Auto power-off.
            hmi_ctx.state = HMI_STATE_UNUSED;
        }
        tim_status = TIM_STD_stop(HMI_TIMER_INSTANCE);
        TIM_exit_error(HMI_ERROR_BASE_TIM);
    }
    // Turn bus interface and HMI off.
    POWER_disable(POWER_REQUESTER_ID_HMI, POWER_DOMAIN_HMI);
    POWER_disable(POWER_REQUESTER_ID_HMI, POWER_DOMAIN_RS485);
    // Disable interrupts.
    _HMI_disable_irq();
    return status;
errors:
    // Print error on screen.
    hmi_ctx.status = status;
    _HMI_update(HMI_SCREEN_ERROR, 1, 1);
    // Delay and exit.
    LPTIM_delay_milliseconds(HMI_UNUSED_DURATION_THRESHOLD_MS, LPTIM_DELAY_MODE_STOP);
    // Stop timer.
    TIM_STD_stop(HMI_TIMER_INSTANCE);
    // Turn bus interface and HMI off.
    POWER_disable(POWER_REQUESTER_ID_HMI, POWER_DOMAIN_HMI);
    POWER_disable(POWER_REQUESTER_ID_HMI, POWER_DOMAIN_RS485);
    // Disable interrupts.
    _HMI_disable_irq();
end:
    return status;
}
