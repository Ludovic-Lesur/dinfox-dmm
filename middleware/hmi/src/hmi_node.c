/*
 * hmi_node.c
 *
 *  Created on: Dec 18, 2024
 *      Author: ludo
 */

#include "hmi_node.h"

#include "common_registers.h"
#include "error.h"
#include "hmi.h"
#include "hmi_bpsm.h"
#include "hmi_ddrm.h"
#include "hmi_dmm.h"
#include "hmi_gpsm.h"
#include "hmi_lvrm.h"
#include "hmi_mpmcm.h"
#include "hmi_r4s8cr.h"
#include "hmi_sm.h"
#include "hmi_uhfm.h"
#include "node.h"
#include "sh1106.h"
#include "string.h"
#include "swreg.h"
#include "types.h"
#include "uhfm_registers.h"
#include "una.h"

/*** HMI NODE local macros ***/

#define HMI_NODE_LINE_WIDTH_CHAR    SH1106_SCREEN_WIDTH_CHAR

#define HMI_NODE_LINE_INDEX_MAX     32
#define HMI_NODE_FIELD_SIZE_CHAR    5

/*** HMI NODE local structures ***/

/*!******************************************************************
 * \enum HMI_NODE_line_type_t
 * \brief Node data formats list.
 *******************************************************************/
typedef struct {
    UNA_convert_physical_data_t convert_physical_data_pfn;
    UNA_get_physical_data_t get_physical_data_pfn;
    STRING_format_t print_format;
    uint8_t print_prefix;
    char_t* unit;
    uint32_t error_value;
} HMI_NODE_data_format_t;

/*******************************************************************/
typedef struct {
    HMI_NODE_line_t* lines_list;
    uint8_t number_of_lines;
} HMI_NODE_descriptor_t;

/*******************************************************************/
typedef struct {
    char_t lines_name[HMI_NODE_LINE_INDEX_MAX][HMI_NODE_LINE_WIDTH_CHAR];
    char_t lines_value[HMI_NODE_LINE_INDEX_MAX][HMI_NODE_LINE_WIDTH_CHAR];
} HMI_NODE_context_t;

/*** HMI NODE local global variables ***/

static const HMI_NODE_data_format_t HMI_NODE_DATA_FORMAT[HMI_NODE_DATA_TYPE_LAST] = {
    { NULL, NULL, STRING_FORMAT_DECIMAL, 0, NULL, UNA_REGISTER_MASK_ALL },
    { NULL, NULL, STRING_FORMAT_HEXADECIMAL, 1, NULL, UNA_REGISTER_MASK_ALL },
    { NULL, NULL, STRING_FORMAT_HEXADECIMAL, 0, NULL, UNA_REGISTER_MASK_ALL },
    { NULL, NULL, STRING_FORMAT_DECIMAL, 0, NULL, UNA_VERSION_ERROR_VALUE },
    { NULL, NULL, STRING_FORMAT_DECIMAL, 0, NULL, UNA_VERSION_ERROR_VALUE },
    { NULL, NULL, STRING_FORMAT_DECIMAL, 0, NULL, UNA_BIT_ERROR },
    { &UNA_convert_seconds, &UNA_get_seconds, STRING_FORMAT_DECIMAL, 0, "s", UNA_TIME_ERROR_VALUE },
    { &UNA_convert_year, &UNA_get_year, STRING_FORMAT_DECIMAL, 0, NULL, UNA_YEAR_ERROR_VALUE },
    { &UNA_convert_degrees, &UNA_get_degrees, STRING_FORMAT_DECIMAL, 0, "|C", UNA_TEMPERATURE_ERROR_VALUE },
    { NULL, NULL, STRING_FORMAT_DECIMAL, 0, "%", UNA_HUMIDITY_ERROR_VALUE },
    { &UNA_convert_mv, &UNA_get_mv, STRING_FORMAT_DECIMAL, 0, "V", UNA_VOLTAGE_ERROR_VALUE },
    { &UNA_convert_ua, &UNA_get_ua, STRING_FORMAT_DECIMAL, 0, "mA", UNA_CURRENT_ERROR_VALUE },
    { &UNA_convert_mw_mva, &UNA_get_mw_mva, STRING_FORMAT_DECIMAL, 0, "W", UNA_ELECTRICAL_POWER_ERROR_VALUE },
    { &UNA_convert_mw_mva, &UNA_get_mw_mva, STRING_FORMAT_DECIMAL, 0, "VA", UNA_ELECTRICAL_ENERGY_ERROR_VALUE },
    { &UNA_convert_mwh_mvah, &UNA_get_mwh_mvah, STRING_FORMAT_DECIMAL, 0, "Wh", UNA_ELECTRICAL_POWER_ERROR_VALUE },
    { &UNA_convert_mwh_mvah, &UNA_get_mwh_mvah, STRING_FORMAT_DECIMAL, 0, "VAh", UNA_ELECTRICAL_ENERGY_ERROR_VALUE },
    { &UNA_convert_power_factor, &UNA_get_power_factor, STRING_FORMAT_DECIMAL, 0, NULL, UNA_POWER_FACTOR_ERROR_VALUE },
    { &UNA_convert_dbm, &UNA_get_dbm, STRING_FORMAT_DECIMAL, 0, "dBm", UNA_RF_POWER_ERROR_VALUE },
    { NULL, NULL, STRING_FORMAT_DECIMAL, 0, "Hz", UNA_MAINS_FREQUENCY_ERROR_VALUE },
};

static const HMI_NODE_descriptor_t HMI_NODE_DESCRIPTOR[UNA_BOARD_ID_LAST] = {
    { (HMI_NODE_line_t*) HMI_LVRM_LINE, HMI_LVRM_LINE_INDEX_LAST },
    { (HMI_NODE_line_t*) HMI_BPSM_LINE, HMI_BPSM_LINE_INDEX_LAST },
    { (HMI_NODE_line_t*) HMI_DDRM_LINE, HMI_DDRM_LINE_INDEX_LAST },
    { (HMI_NODE_line_t*) HMI_UHFM_LINE, HMI_UHFM_LINE_INDEX_LAST },
    { (HMI_NODE_line_t*) HMI_GPSM_LINE, HMI_GPSM_LINE_INDEX_LAST },
    { (HMI_NODE_line_t*) HMI_SM_LINE, HMI_SM_LINE_INDEX_LAST },
    { NULL, 0 },
    { NULL, 0 },
    { (HMI_NODE_line_t*) HMI_DMM_LINE, HMI_DMM_LINE_INDEX_LAST },
    { (HMI_NODE_line_t*) HMI_MPMCM_LINE, HMI_MPMCM_LINE_INDEX_LAST },
    { (HMI_NODE_line_t*) HMI_R4S8CR_LINE, HMI_R4S8CR_LINE_INDEX_LAST }
};

static HMI_NODE_context_t hmi_node_ctx;

/*** HMI NODE local functions ***/

/*******************************************************************/
#define _HMI_NODE_check_node(void) { \
    if (node == NULL) { \
        status = HMI_ERROR_NULL_PARAMETER; \
        goto errors; \
    } \
    if ((node->board_id) >= UNA_BOARD_ID_LAST) { \
        status = HMI_ERROR_NODE_BOARD_ID; \
        goto errors; \
    } \
    if ((HMI_NODE_DESCRIPTOR[node -> board_id].number_of_lines) == 0) { \
        status = HMI_ERROR_NODE_NOT_SUPPORTED; \
        goto errors; \
    } \
}

/*******************************************************************/
#define _HMI_NODE_line_name_add_string(str) { \
    string_status = STRING_append_string((char_t*) &(hmi_node_ctx.lines_name[line_index]), HMI_NODE_LINE_WIDTH_CHAR, str, &buffer_size); \
    STRING_exit_error(HMI_ERROR_BASE_STRING); \
}

/*******************************************************************/
#define _HMI_NODE_line_value_flush(void) { \
    uint8_t char_idx = 0; \
    for (char_idx = 0; char_idx < HMI_NODE_LINE_WIDTH_CHAR; char_idx++) { \
        hmi_node_ctx.lines_value[line_index][char_idx] = STRING_CHAR_NULL; \
    } \
    buffer_size = 0; \
}

/*******************************************************************/
#define _HMI_NODE_line_value_add_string(str) { \
    string_status = STRING_append_string((char_t*) &(hmi_node_ctx.lines_value[line_index]), HMI_NODE_LINE_WIDTH_CHAR, str, &buffer_size); \
    STRING_exit_error(HMI_ERROR_BASE_STRING); \
}

/*******************************************************************/
#define _HMI_NODE_line_value_add_integer(value, format, print_prefix) { \
    string_status = STRING_append_integer((char_t*) &(hmi_node_ctx.lines_value[line_index]), HMI_NODE_LINE_WIDTH_CHAR, value, format, print_prefix, &buffer_size); \
    STRING_exit_error(HMI_ERROR_BASE_STRING); \
}

/*******************************************************************/
static void _HMI_NODE_flush_line(uint8_t line_index) {
    // Local variables.
    uint8_t idx = 0;
    // Char loop.
    for (idx=0 ; idx<HMI_NODE_LINE_WIDTH_CHAR ; idx++) {
        hmi_node_ctx.lines_name[line_index][idx] = STRING_CHAR_NULL;
        hmi_node_ctx.lines_value[line_index][idx] = STRING_CHAR_NULL;
    }
}

/*******************************************************************/
static void _HMI_NODE_flush_all_data_value(void) {
    // Local variables.
    uint8_t idx = 0;
    // Reset string data.
    for (idx=0 ; idx<HMI_NODE_LINE_INDEX_MAX ; idx++) {
        _HMI_NODE_flush_line(idx);
    }
}

/*******************************************************************/
static HMI_status_t _HMI_build_line(uint8_t line_index, char_t* name, HMI_NODE_data_type_t data_type, uint32_t field_value) {
    // Local variables.
    HMI_status_t status = HMI_SUCCESS;
    STRING_status_t string_status = STRING_SUCCESS;
    STRING_format_t print_format = STRING_FORMAT_LAST;
    uint8_t print_prefix = 0;
    char_t* unit = NULL;
    int32_t physical_data = 0;
    char_t physical_data_str[HMI_NODE_FIELD_SIZE_CHAR + 1];
    uint32_t tmp_u32 = 0;
    uint32_t buffer_size = 0;
    uint8_t idx = 0;
    // Reset string.
    for (idx = 0; idx < (HMI_NODE_FIELD_SIZE_CHAR + 1); idx++) {
        physical_data_str[idx] = STRING_CHAR_NULL;
    }
    // Check data type.
    if (data_type >= HMI_NODE_DATA_TYPE_LAST) {
        status = HMI_ERROR_NODE_HMI_NODE_DATA_TYPE;
        goto errors;
    }
    // Get string format.
    print_format = HMI_NODE_DATA_FORMAT[data_type].print_format;
    print_prefix = HMI_NODE_DATA_FORMAT[data_type].print_prefix;
    unit = HMI_NODE_DATA_FORMAT[data_type].unit;
    // Print field name.
    _HMI_NODE_line_name_add_string(name);
    _HMI_NODE_line_value_flush();
    _HMI_NODE_line_value_add_string("ERROR");
    // Check error value.
    if (field_value == HMI_NODE_DATA_FORMAT[data_type].error_value) goto errors;
    // Specific print according to data type.
    _HMI_NODE_line_value_flush();
    switch (data_type) {
    case HMI_NODE_DATA_TYPE_EP_ID:
        // Print ID in reverse order.
        for (idx=0 ; idx<UHFM_EP_ID_SIZE_BYTES ; idx++) {
            _HMI_NODE_line_value_add_integer(((field_value >> (idx << 3)) & 0xFF), STRING_FORMAT_HEXADECIMAL, ((idx == 0) ? 1 : 0));
        }
        break;
    case HMI_NODE_DATA_TYPE_HARDWARE_VERSION:
        // Print HW version.
        tmp_u32 = SWREG_read_field(field_value, COMMON_REGISTER_HW_VERSION_MASK_MAJOR);
        _HMI_NODE_line_value_add_integer(tmp_u32, print_format, print_prefix);
        _HMI_NODE_line_value_add_string(".");
        tmp_u32 = SWREG_read_field(field_value, COMMON_REGISTER_HW_VERSION_MASK_MINOR);
        _HMI_NODE_line_value_add_integer(tmp_u32, print_format, print_prefix);
        break;
    case HMI_NODE_DATA_TYPE_SOFTWARE_VERSION:
        // Print HW version.
        tmp_u32 = SWREG_read_field(field_value, COMMON_REGISTER_SW_VERSION_0_MASK_MAJOR);
        _HMI_NODE_line_value_add_integer(tmp_u32, print_format, print_prefix);
        _HMI_NODE_line_value_add_string(".");
        tmp_u32 = SWREG_read_field(field_value, COMMON_REGISTER_SW_VERSION_0_MASK_MINOR);
        _HMI_NODE_line_value_add_integer(tmp_u32, print_format, print_prefix);
        _HMI_NODE_line_value_add_string(".");
        tmp_u32 = SWREG_read_field(field_value, COMMON_REGISTER_SW_VERSION_0_MASK_COMMIT_INDEX);
        _HMI_NODE_line_value_add_integer(tmp_u32, print_format, print_prefix);
        if (SWREG_read_field(field_value, COMMON_REGISTER_SW_VERSION_0_MASK_DTYF) != 0) {
            _HMI_NODE_line_value_add_string(".d");
        }
        break;
    case HMI_NODE_DATA_TYPE_BIT:
        switch (field_value) {
        case UNA_BIT_0:
            _HMI_NODE_line_value_add_string("OFF");
            break;
        case UNA_BIT_1:
            _HMI_NODE_line_value_add_string("ON");
            break;
        case UNA_BIT_FORCED_HARDWARE:
            _HMI_NODE_line_value_add_string("HW");
            break;
        default:
            _HMI_NODE_line_value_add_string("ERROR");
            break;
        }
        break;
    default:
        // Check conversion function.
        if (HMI_NODE_DATA_FORMAT[data_type].get_physical_data_pfn != NULL) {
            // Convert UNA representation to physical data.
            physical_data = HMI_NODE_DATA_FORMAT[data_type].get_physical_data_pfn(field_value);
            // Convert to floating decimal string if needed.
            if ((data_type == HMI_NODE_DATA_TYPE_VOLTAGE) || (data_type == HMI_NODE_DATA_TYPE_CURRENT)) {
                // Convert to 5 digits string.
                string_status = STRING_integer_to_floating_decimal_string(physical_data, 3, HMI_NODE_FIELD_SIZE_CHAR, physical_data_str);
                STRING_exit_error(HMI_ERROR_BASE_STRING);
                // Add output string.
                _HMI_NODE_line_value_add_string(physical_data_str);
            }
            else {
                _HMI_NODE_line_value_add_integer(physical_data, print_format, print_prefix);
            }
        }
        else {
            // Print raw field value without conversion.
            _HMI_NODE_line_value_add_integer(field_value, print_format, print_prefix);
        }
    }
    // Print unit.
    if (unit != NULL) {
        _HMI_NODE_line_value_add_string(" ");
        _HMI_NODE_line_value_add_string(unit);
    }
errors:
    if (status != HMI_SUCCESS) {
        _HMI_NODE_line_value_flush();
        _HMI_NODE_line_value_add_string("ERROR");
    }
    return status;
}

/*** HMI NODE functions ***/

/*******************************************************************/
HMI_status_t HMI_NODE_write_line(UNA_node_t* node, uint8_t line_index, int32_t field_value) {
    // Local variables.
    HMI_status_t status = HMI_SUCCESS;
    NODE_status_t node_status = NODE_SUCCESS;
    UNA_access_status_t write_status;
    HMI_NODE_data_type_t data_type;
    uint8_t reg_addr = 0;
    uint32_t reg_value = 0;
    uint32_t field_mask = 0;
    uint32_t field_una_value = (uint32_t) field_value;
    uint32_t unused_reg_mask = 0;
    // Check node and board ID.
    _HMI_NODE_check_node();
    // Convert line index to register address.
    data_type = HMI_NODE_DESCRIPTOR[node->board_id].lines_list[line_index].data_type;
    reg_addr = HMI_NODE_DESCRIPTOR[node->board_id].lines_list[line_index].write_reg_addr;
    field_mask = HMI_NODE_DESCRIPTOR[node->board_id].lines_list[line_index].write_field_mask;
    // Convert to UNA representation if needed.
    if (data_type < HMI_NODE_DATA_TYPE_LAST) {
        // Check conversion function.
        if (HMI_NODE_DATA_FORMAT[data_type].convert_physical_data_pfn != NULL) {
            field_una_value = HMI_NODE_DATA_FORMAT[data_type].convert_physical_data_pfn(field_value);
        }
    }
    // Update register value.
    SWREG_write_field(&reg_value, &unused_reg_mask, field_una_value, field_mask);
    // Execute write operation.
    node_status = NODE_write_register(node, reg_addr, reg_value, field_mask, &write_status);
    NODE_exit_error(HMI_ERROR_BASE_NODE);
    // Check node access status.
    if (write_status.flags != 0) {
       status = HMI_ERROR_NODE_WRITE_ACCESS;
       goto errors;
    }
errors:
    return status;
}

/*******************************************************************/
HMI_status_t HMI_NODE_read_line(UNA_node_t* node, uint8_t line_index) {
    // Local variables.
    HMI_status_t status = HMI_SUCCESS;
    NODE_status_t node_status = NODE_SUCCESS;
    UNA_access_status_t read_status;
    char_t* name = NULL;
    HMI_NODE_data_type_t data_type = HMI_NODE_DATA_TYPE_LAST;
    uint8_t reg_addr = 0;
    uint32_t reg_value = 0;
    uint32_t field_mask = 0;
    uint32_t field_value = 0;
    // Check node and board ID.
    _HMI_NODE_check_node();
    // Flush local buffer.
    _HMI_NODE_flush_line(line_index);
    // Get line parameters.
    name = HMI_NODE_DESCRIPTOR[node->board_id].lines_list[line_index].name;
    data_type = HMI_NODE_DESCRIPTOR[node->board_id].lines_list[line_index].data_type;
    reg_addr = HMI_NODE_DESCRIPTOR[node->board_id].lines_list[line_index].read_reg_addr;
    field_mask = HMI_NODE_DESCRIPTOR[node->board_id].lines_list[line_index].read_field_mask;
    // Execute read operation.
    node_status = NODE_read_register(node, reg_addr, &reg_value, &read_status);
    NODE_exit_error(HMI_ERROR_BASE_NODE);
    // Check node access status.
    if (read_status.flags != 0) {
        status = HMI_ERROR_NODE_READ_ACCESS;
        goto errors;
    }
    // Compute field value.
    field_value = SWREG_read_field(reg_value, field_mask);
    // Build line.
    status = _HMI_build_line(line_index, name, data_type, field_value);
    if (status != HMI_SUCCESS) goto errors;
errors:
    return status;
}

/*******************************************************************/
HMI_status_t HMI_NODE_read_line_all(UNA_node_t* node) {
    // Local variables.
    HMI_status_t status = HMI_SUCCESS;
    NODE_status_t node_status = NODE_SUCCESS;
    UNA_access_status_t write_status;
    uint8_t idx = 0;
    // Check board ID.
    _HMI_NODE_check_node();
    // Reset buffers.
    _HMI_NODE_flush_all_data_value();
    // Perform node measurements.
    node_status = NODE_perform_measurements(node, &write_status);
    NODE_exit_error(HMI_ERROR_BASE_NODE);
    // Check node access status.
    if (write_status.flags != 0) {
        status = HMI_ERROR_NODE_MEASUREMENTS;
        goto errors;
    }
    // String data loop.
    for (idx=0 ; idx<(HMI_NODE_DESCRIPTOR[node -> board_id].number_of_lines) ; idx++) {
        status = HMI_NODE_read_line(node, idx);
        if (status != HMI_SUCCESS) goto errors;
    }
errors:
    return status;
}

/*******************************************************************/
HMI_status_t HMI_NODE_get_line_data(UNA_node_t* node, uint8_t line_index, char_t** line_name_ptr, char_t** line_value_ptr) {
    // Local variables.
    HMI_status_t status = HMI_SUCCESS;
    // Check parameters.
    _HMI_NODE_check_node();
    // Check index.
    if (line_index >= (HMI_NODE_DESCRIPTOR[node -> board_id].number_of_lines)) { \
        status = HMI_ERROR_NODE_LINE_INDEX;
        goto errors;
    }
    // Update pointers.
    (*line_name_ptr) = (char_t*) hmi_node_ctx.lines_name[line_index];
    (*line_value_ptr) = (char_t*) hmi_node_ctx.lines_value[line_index];
errors:
    return status;
}

/*******************************************************************/
HMI_status_t HMI_NODE_get_last_line_index(UNA_node_t* node, uint8_t* last_line_index) {
    // Local variables.
    HMI_status_t status = HMI_SUCCESS;
    // Check board ID.
    _HMI_NODE_check_node();
    // Get name of the corresponding board ID.
    (*last_line_index) = HMI_NODE_DESCRIPTOR[node->board_id].number_of_lines;
errors:
    return status;
}
