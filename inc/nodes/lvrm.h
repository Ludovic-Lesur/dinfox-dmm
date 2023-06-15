/*
 * lvrm.h
 *
 *  Created on: 21 jan. 2023
 *      Author: Ludo
 */

#ifndef __LVRM_H__
#define __LVRM_H__

#include "at_bus.h"
#include "common.h"
#include "lvrm_reg.h"
#include "node.h"
#include "string.h"
#include "types.h"

/*** LVRM structures ***/

typedef enum {
	LVRM_LINE_DATA_INDEX_VCOM = COMMON_LINE_DATA_INDEX_LAST,
	LVRM_LINE_DATA_INDEX_VOUT,
	LVRM_LINE_DATA_INDEX_IOUT,
	LVRM_LINE_DATA_INDEX_RLST,
	LVRM_LINE_DATA_INDEX_LAST,
} LVRM_line_data_index_t;

/*** LVRM global variables ***/

static const uint32_t LVRM_REG_WRITE_TIMEOUT_MS[LVRM_NUMBER_OF_SPECIFIC_REG] = {
	2000,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS
};

/*** LVRM functions ***/

NODE_status_t LVRM_write_line_data(NODE_line_data_write_t* line_data_write, NODE_access_status_t* write_status);
NODE_status_t LVRM_read_line_data(NODE_line_data_read_t* line_data_read, NODE_access_status_t* read_status);
NODE_status_t LVRM_build_sigfox_ul_payload(NODE_ul_payload_update_t* ul_payload_update);

#endif /* __LVRM_H__ */
