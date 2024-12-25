/*
 * i2c_address.h
 *
 *  Created on: 15 dec. 2024
 *      Author: Ludo
 */

#ifndef __I2C_ADDRESS_H__
#define __I2C_ADDRESS_H__

/*!******************************************************************
 * \enum I2C_address_mapping_t
 * \brief I2C slaves address mapping.
 *******************************************************************/
typedef enum {
    I2C_ADDRESS_SH1106_HMI = 0x3C
} I2C_address_mapping_t;

#endif /* __I2C_ADDRESS_H__ */
