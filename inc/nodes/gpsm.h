/*
 * gpsm.h
 *
 *  Created on: 16 apr. 2023
 *      Author: Ludo
 */

#ifndef __GPSM_H__
#define __GPSM_H__

#include "at_bus.h"
#include "common.h"
#include "gpsm_reg.h"
#include "node.h"
#include "string.h"
#include "types.h"

/*** GPSM structures ***/

typedef enum {
	GPSM_LINE_DATA_INDEX_VGPS = COMMON_LINE_DATA_INDEX_LAST,
	GPSM_LINE_DATA_INDEX_VANT,
	GPSM_LINE_DATA_INDEX_LAST,
} GPSM_line_data_index_t;

/*** GPSM global variables ***/

static const uint32_t GPSM_REG_WRITE_TIMEOUT_MS[GPSM_NUMBER_OF_SPECIFIC_REG] = {
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS,
	AT_BUS_DEFAULT_TIMEOUT_MS
};

/*** GPSM functions ***/

NODE_status_t GPSM_write_line_data(NODE_line_data_write_t* line_data_write, NODE_access_status_t* write_status);
NODE_status_t GPSM_read_line_data(NODE_line_data_read_t* line_data_read, NODE_access_status_t* read_status);
NODE_status_t GPSM_build_sigfox_ul_payload(NODE_ul_payload_update_t* ul_payload_update);

#endif /* __GPSM_H__ */
