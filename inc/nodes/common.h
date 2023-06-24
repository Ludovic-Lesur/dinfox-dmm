/*
 * common.h
 *
 *  Created on: 12 nov 2022
 *      Author: Ludo
 */

#ifndef __COMMON_H__
#define __COMMON_H__

#include "at_bus.h"
#include "common_reg.h"
#include "node.h"
#include "string.h"
#include "types.h"
#include "xm.h"

/*** COMMON macros ***/

#define COMMON_REG_ERROR_VALUE \
	0x00000000, \
	0x00000000, \
	0x00000000, \
	0x00000000, \
	0x00000000, \
	0x00000000, \
	0x00000000, \
	((DINFOX_TEMPERATURE_ERROR_VALUE << 16) | (DINFOX_VOLTAGE_ERROR_VALUE << 0)), \

/*** COMMON structures ***/

typedef enum {
	COMMON_LINE_DATA_INDEX_HW_VERSION = 0,
	COMMON_LINE_DATA_INDEX_SW_VERSION,
	COMMON_LINE_DATA_INDEX_RESET_REASON,
	COMMON_LINE_DATA_INDEX_VMCU_MV,
	COMMON_LINE_DATA_INDEX_TMCU_DEGREES,
	COMMON_LINE_DATA_INDEX_LAST
} COMMON_line_data_index_t;

static const uint32_t COMMON_REG_WRITE_TIMEOUT_MS[COMMON_REG_ADDR_LAST] = {
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	5000,
	AT_BUS_DEFAULT_TIMEOUT_MS
};

/*** COMMON functions ***/

NODE_status_t COMMON_write_line_data(NODE_line_data_write_t* line_data_write);
NODE_status_t COMMON_read_line_data(NODE_line_data_read_t* line_data_read, XM_node_registers_t* node_reg);
NODE_status_t COMMON_build_sigfox_payload_startup(NODE_ul_payload_update_t* ul_payload_update, XM_node_registers_t* node_reg);

#endif /* __COMMON_H__ */
