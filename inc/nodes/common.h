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
#include "node_common.h"
#include "string.h"
#include "types.h"
#include "xm.h"

/*** COMMON macros ***/

#define COMMON_REG_ERROR_VALUE_LIST \
	0x00000000, \
	0x00000000, \
	0x00000000, \
	0x00000000, \
	0x00000000, \
	0x00000000, \
	0x00000000, \
	((DINFOX_TEMPERATURE_ERROR_VALUE << 16) | (DINFOX_VOLTAGE_ERROR_VALUE << 0)), \

#define COMMON_REG_WRITE_TIMEOUT_MS_LIST \
	AT_BUS_DEFAULT_TIMEOUT_MS, \
	AT_BUS_DEFAULT_TIMEOUT_MS, \
	AT_BUS_DEFAULT_TIMEOUT_MS, \
	AT_BUS_DEFAULT_TIMEOUT_MS, \
	AT_BUS_DEFAULT_TIMEOUT_MS, \
	AT_BUS_DEFAULT_TIMEOUT_MS, \
	5000, \
	AT_BUS_DEFAULT_TIMEOUT_MS, \

/*** COMMON structures ***/

/*!******************************************************************
 * \enum COMMON_line_data_index_t
 * \brief Common registers screen data lines index.
 *******************************************************************/
typedef enum {
	COMMON_LINE_DATA_INDEX_HW_VERSION = 0,
	COMMON_LINE_DATA_INDEX_SW_VERSION,
	COMMON_LINE_DATA_INDEX_RESET_REASON,
	COMMON_LINE_DATA_INDEX_VMCU_MV,
	COMMON_LINE_DATA_INDEX_TMCU_DEGREES,
	COMMON_LINE_DATA_INDEX_LAST
} COMMON_line_data_index_t;

/*** COMMON global variables ***/

static const uint32_t COMMON_REG_WRITE_TIMEOUT_MS[COMMON_REG_ADDR_LAST] = {
	COMMON_REG_WRITE_TIMEOUT_MS_LIST
};

/*** COMMON functions ***/

/*!******************************************************************
 * \fn NODE_status_t COMMON_write_line_data(NODE_line_data_write_t* line_data_write, XM_node_registers_t* node_reg, NODE_access_status_t* write_status)
 * \brief Write corresponding node common register of screen data line.
 * \param[in]  	line_data_write: Pointer to the writing operation parameters.
 * \param[out] 	write_status: Pointer to the writing operation status.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t COMMON_write_line_data(NODE_line_data_write_t* line_data_write, NODE_access_status_t* write_status);

/*!******************************************************************
 * \fn NODE_status_t COMMON_read_line_data(NODE_line_data_read_t* line_data_read, XM_node_registers_t* node_reg, NODE_access_status_t* read_status)
 * \brief Read corresponding node common register of screen data line.
 * \param[in]  	line_data_read: Pointer to the reading operation parameters.
 * \param[in]	node_reg: Pointer to the node registers and error values.
 * \param[out] 	read_status: Pointer to the reading operation status.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t COMMON_read_line_data(NODE_line_data_read_t* line_data_read, XM_node_registers_t* node_reg, NODE_access_status_t* read_status);

/*!******************************************************************
 * \fn NODE_status_t COMMON_check_event_driven_payloads(NODE_ul_payload_t* node_ul_payload, XM_node_registers_t* node_reg)
 * \brief Check common flags and build associated payloads.
 * \param[in]	node_reg: Pointer to the node registers and error values.
 * \param[out] 	node_ul_payload: Pointer to the Sigfox uplink payload.
 * \retval		Function execution status.
 *******************************************************************/
NODE_status_t COMMON_check_event_driven_payloads(NODE_ul_payload_t* node_ul_payload, XM_node_registers_t* node_reg);

#endif /* __COMMON_H__ */
