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
NODE_status_t LVRM_perform_measurements(void);
NODE_status_t LVRM_unstack_string_data(char_t** measurement_name_ptr, char_t** measurement_value_ptr);
NODE_status_t LVRM_get_sigfox_payload(uint8_t* ul_payload, uint8_t* ul_payload_size);
NODE_status_t LVRM_write(uint8_t register_address, uint8_t value);

#endif /* __LVRM_H__ */
