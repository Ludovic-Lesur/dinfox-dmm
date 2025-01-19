/*
 * nvm_address.h
 *
 *  Created on: 15 dec. 2024
 *      Author: Ludo
 */

#ifndef __NVM_ADDRESS_H__
#define __NVM_ADDRESS_H__

/*!******************************************************************
 * \enum NVM_address_mapping_t
 * \brief NVM address mapping.
 *******************************************************************/
typedef enum {
    NVM_ADDRESS_UNA_SELF_ADDRESS = 0,
    NVM_ADDRESS_UNA_REGISTERS = 0x40,
} NVM_address_mapping_t;

#endif /* __NVM_ADDRESS_H__ */
