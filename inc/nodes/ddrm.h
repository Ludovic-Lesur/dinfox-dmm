/*
 * ddrm.h
 *
 *  Created on: 26 feb. 2023
 *      Author: Ludo
 */

#ifndef __DDRM_H__
#define __DDRM_H__

#include "at_bus.h"
#include "common.h"
#include "ddrm_reg.h"
#include "node.h"
#include "string.h"
#include "types.h"

/*** DDRM structures ***/

typedef enum {
	DDRM_LINE_DATA_INDEX_VIN = COMMON_LINE_DATA_INDEX_LAST,
	DDRM_LINE_DATA_INDEX_VOUT,
	DDRM_LINE_DATA_INDEX_IOUT,
	DDRM_LINE_DATA_INDEX_DDEN,
	DDRM_LINE_DATA_INDEX_LAST,
} DDRM_line_data_index_t;

/*** DDRM global variables ***/

static const uint32_t DDRM_REG_WRITE_TIMEOUT_MS[DDRM_NUMBER_OF_SPECIFIC_REG] = {
	1000,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS
};

/*** DDRM functions ***/

NODE_status_t DDRM_write_line_data(NODE_line_data_write_t* line_data_write, NODE_access_status_t* write_status);
NODE_status_t DDRM_read_line_data(NODE_line_data_read_t* line_data_read, NODE_access_status_t* read_status);
NODE_status_t DDRM_build_sigfox_ul_payload(NODE_ul_payload_update_t* ul_payload_update);

#endif /* __DDRM_H__ */
