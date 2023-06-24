/*
 * r4s8cr.h
 *
 *  Created on: 2 feb. 2023
 *      Author: Ludo
 */

#ifndef __R4S8CR_H__
#define __R4S8CR_H__

#include "node.h"
#include "r4s8cr_reg.h"

/*** R4S8CR macros ***/

#define R4S8CR_DEFAULT_TIMEOUT_MS	100

/*** R4S8CR structures ***/

typedef enum {
	R4S8CR_LINE_DATA_INDEX_R1ST = 0,
	R4S8CR_LINE_DATA_INDEX_R2ST,
	R4S8CR_LINE_DATA_INDEX_R3ST,
	R4S8CR_LINE_DATA_INDEX_R4ST,
	R4S8CR_LINE_DATA_INDEX_R5ST,
	R4S8CR_LINE_DATA_INDEX_R6ST,
	R4S8CR_LINE_DATA_INDEX_R7ST,
	R4S8CR_LINE_DATA_INDEX_R8ST,
	R4S8CR_LINE_DATA_INDEX_LAST,
} R4S8CR_line_data_index_t;

static const uint32_t R4S8CR_REG_WRITE_TIMEOUT_MS[R4S8CR_REG_ADDR_LAST] = {
	R4S8CR_DEFAULT_TIMEOUT_MS
};

/*** R4S8CR functions ***/

void R4S8CR_init_registers(void);

NODE_status_t R4S8CR_write_register(NODE_access_parameters_t* write_params, uint32_t reg_value, uint32_t reg_mask, NODE_access_status_t* write_status);
NODE_status_t R4S8CR_read_register(NODE_access_parameters_t* read_params, uint32_t* reg_value, NODE_access_status_t* read_status);

NODE_status_t R4S8CR_scan(NODE_t* nodes_list, uint8_t nodes_list_size, uint8_t* nodes_count);

NODE_status_t R4S8CR_write_line_data(NODE_line_data_write_t* line_data_write, NODE_access_status_t* write_status);
NODE_status_t R4S8CR_read_line_data(NODE_line_data_read_t* line_data_read, NODE_access_status_t* read_status);
NODE_status_t R4S8CR_build_sigfox_ul_payload(NODE_ul_payload_update_t* ul_payload_update);

void R4S8CR_fill_rx_buffer(uint8_t rx_byte);

#endif /* __R4S8CR_H__ */
