/*
 * lvrm.h
 *
 *  Created on: 21 jan. 2023
 *      Author: Ludo
 */

#ifndef __LVRM_H__
#define __LVRM_H__

#include "dinfox.h"
#include "node_common.h"
#include "rs485_common.h"
#include "types.h"

/*** LVRM functions ***/

NODE_status_t LVRM_get_board_name(char_t** board_name_ptr);
NODE_status_t LVRM_set_rs485_address(RS485_address_t rs485_address);
NODE_status_t LVRM_update_data(uint8_t string_data_index);
NODE_status_t LVRM_update_all_data(void);
NODE_status_t LVRM_read_string_data(uint8_t string_data_index, char_t** string_data_name_ptr, char_t** string_data_value_ptr);
NODE_status_t LVRM_write_data(uint8_t string_data_index, uint8_t value, RS485_reply_status_t* reply_status);
NODE_status_t LVRM_get_sigfox_payload(uint8_t* ul_payload, uint8_t* ul_payload_size);

#endif /* __LVRM_H__ */
