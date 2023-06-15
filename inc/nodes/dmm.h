/*
 * dmm.h
 *
 *  Created on: 08 jan. 2023
 *      Author: Ludo
 */

#ifndef __DMM_H__
#define __DMM_H__

#include "at_bus.h"
#include "common.h"
#include "dmm_reg.h"
#include "node.h"
#include "node_common.h"
#include "string.h"
#include "types.h"

/*** DMM structures ***/

typedef enum {
	DMM_LINE_DATA_INDEX_VRS = COMMON_LINE_DATA_INDEX_LAST,
	DMM_LINE_DATA_INDEX_VHMI,
	DMM_LINE_DATA_INDEX_VUSB,
	DMM_LINE_DATA_INDEX_NODES_COUNT,
	DMM_LINE_DATA_INDEX_UL_PERIOD,
	DMM_LINE_DATA_INDEX_DL_PERIOD,
	DMM_LINE_DATA_INDEX_LAST,
} DMM_line_data_index_t;

/*** DMM global variables ***/

static const uint32_t DMM_REG_WRITE_TIMEOUT_MS[DMM_NUMBER_OF_SPECIFIC_REG] = {
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS
};

/*** DMM functions ***/

void DMM_init_registers(void);
NODE_status_t DMM_write_register(NODE_access_parameters_t* write_params, uint32_t reg_value, uint32_t reg_mask, NODE_access_status_t* write_status);
NODE_status_t DMM_read_register(NODE_access_parameters_t* read_params, uint32_t* reg_value, NODE_access_status_t* read_status);

NODE_status_t DMM_write_line_data(NODE_line_data_write_t* line_data_write, NODE_access_status_t* write_status);
NODE_status_t DMM_read_line_data(NODE_line_data_read_t* line_data_read, NODE_access_status_t* read_status);
NODE_status_t DMM_build_sigfox_ul_payload(NODE_ul_payload_update_t* ul_payload_update);

#endif /* __DMM_H__ */
