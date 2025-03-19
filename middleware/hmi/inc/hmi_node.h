/*
 * hmi_node.h
 *
 *  Created on: 18 dec. 2024
 *      Author: Ludo
 */

#ifndef __HMI_NODE_H__
#define __HMI_NODE_H__

#include "hmi.h"
#include "strings.h"
#include "types.h"
#include "una.h"

/*** HMI NODE structures ***/

/*!******************************************************************
 * \enum HMI_NODE_data_type_t
 * \brief Node data types list.
 *******************************************************************/
typedef enum {
    HMI_NODE_DATA_TYPE_RAW_DECIMAL = 0,
    HMI_NODE_DATA_TYPE_RAW_HEXADECIMAL,
    HMI_NODE_DATA_TYPE_EP_ID,
    HMI_NODE_DATA_TYPE_HARDWARE_VERSION,
    HMI_NODE_DATA_TYPE_SOFTWARE_VERSION,
    HMI_NODE_DATA_TYPE_BIT,
    HMI_NODE_DATA_TYPE_TIME,
    HMI_NODE_DATA_TYPE_YEAR,
    HMI_NODE_DATA_TYPE_TEMPERATURE,
    HMI_NODE_DATA_TYPE_HUMIDITY,
    HMI_NODE_DATA_TYPE_VOLTAGE,
    HMI_NODE_DATA_TYPE_CURRENT,
    HMI_NODE_DATA_TYPE_ACTIVE_POWER,
    HMI_NODE_DATA_TYPE_APPARENT_POWER,
    HMI_NODE_DATA_TYPE_ACTIVE_ENERGY,
    HMI_NODE_DATA_TYPE_APPARENT_ENERGY,
    HMI_NODE_DATA_TYPE_POWER_FACTOR,
    HMI_NODE_DATA_TYPE_RF_POWER,
    HMI_NODE_DATA_TYPE_FREQUENCY,
    HMI_NODE_DATA_TYPE_LAST
} HMI_NODE_data_type_t;

/*!******************************************************************
 * \enum HMI_NODE_line_t
 * \brief Displayed line data of a node.
 *******************************************************************/
typedef struct {
    // Field name and type.
    char_t* name;
    HMI_NODE_data_type_t data_type;
    // Read parameters.
    uint8_t read_reg_addr;
    uint32_t read_field_mask;
    // Write parameters.
    uint8_t write_reg_addr;
    uint32_t write_field_mask;
} HMI_NODE_line_t;

/*** HMI NODE functions ***/

/*!******************************************************************
 * \fn HMI_status_t HMI_NODE_write_line(UNA_node_t* node, uint8_t line_index, int32_t field_value)
 * \brief Write corresponding node register of screen data line.
 * \param[in]   node: Pointer to the node.
 * \param[in]   line_index: Index of the data line to write.
 * \param[in]   field_value: Value to write in corresponding register field.
 * \retval      Function execution status.
 *******************************************************************/
HMI_status_t HMI_NODE_write_line(UNA_node_t* node, uint8_t line_index, int32_t field_value);

/*!******************************************************************
 * \fn HMI_status_t HMI_NODE_read_line(UNA_node_t* node, uint8_t line_index)
 * \brief Read corresponding node register of screen data line.
 * \param[in]   node: Pointer to the node.
 * \param[in]   line_index: Index of the data line to read.
 * \retval      Function execution status.
 *******************************************************************/
HMI_status_t HMI_NODE_read_line(UNA_node_t* node, uint8_t line_index);

/*!******************************************************************
 * \fn HMI_status_t HMI_NODE_read_line_all(UNA_node_t* node)
 * \brief Read corresponding node registers of all screen data lines.
 * \param[in]   node: Pointer to the node.
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
HMI_status_t HMI_NODE_read_line_all(UNA_node_t* node);

/*!******************************************************************
 * \fn HMI_status_t HMI_NODE_get_line_data(UNA_node_t* node, uint8_t line_index, char_t** line_name_ptr, char_t** line_value_ptr)
 * \brief Read data line.
 * \param[in]   node: Pointer to the node.
 * \param[in]   line_index: Index of the data line to read.
 * \param[out]  line_name_ptr: Pointer to string that will contain the data name.
 * \param[out]  line_value_ptr: Pointer to string that will contain the data value.
 * \retval      Function execution status.
 *******************************************************************/
HMI_status_t HMI_NODE_get_line_data(UNA_node_t* node, uint8_t line_index, char_t** line_name_ptr, char_t** line_value_ptr);

/*!******************************************************************
 * \fn HMI_status_t HMI_NODE_get_last_line_index(UNA_node_t* node, uint8_t* last_line_index)
 * \brief Get the number of data lines.
 * \param[in]   node: Pointer to the node.
 * \param[out]  last_line_index: Pointer to byte that will contain the number of displayed data lines.
 * \retval      Function execution status.
 *******************************************************************/
HMI_status_t HMI_NODE_get_last_line_index(UNA_node_t* node, uint8_t* last_line_index);

#endif /* __HMI_NODE_H__ */
