/*
 * bpsm.h
 *
 *  Created on: 26 feb. 2023
 *      Author: Ludo
 */

#ifndef __BPSM_H__
#define __BPSM_H__

#include "at_bus.h"
#include "common.h"
#include "bpsm_reg.h"
#include "node.h"
#include "string.h"
#include "types.h"

/*** BPSM structures ***/

typedef enum {
	BPSM_LINE_DATA_INDEX_VSRC = COMMON_LINE_DATA_INDEX_LAST,
	BPSM_LINE_DATA_INDEX_VSTR,
	BPSM_LINE_DATA_INDEX_VBKP,
	BPSM_LINE_DATA_INDEX_CHEN,
	BPSM_LINE_DATA_INDEX_CHST,
	BPSM_LINE_DATA_INDEX_BKEN,
	BPSM_LINE_DATA_INDEX_LAST,
} BPSM_line_data_index_t;

/*** BPSM global variables ***/

static const uint32_t BPSM_REG_WRITE_TIMEOUT_MS[BPSM_NUMBER_OF_SPECIFIC_REG] = {
	1000,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS
};

/*** BPSM functions ***/

NODE_status_t BPSM_write_line_data(NODE_line_data_write_t* line_data_write, NODE_access_status_t* write_status);
NODE_status_t BPSM_read_line_data(NODE_line_data_read_t* line_data_read, NODE_access_status_t* read_status);
NODE_status_t BPSM_build_sigfox_ul_payload(NODE_ul_payload_update_t* ul_payload_update);

#endif /* __BPSM_H__ */
