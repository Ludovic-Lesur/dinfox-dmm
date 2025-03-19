/*
 * dmm.h
 *
 *  Created on: 08 jan. 2023
 *      Author: Ludo
 */

#ifndef __UNA_DMM_H__
#define __UNA_DMM_H__

#include "nvm.h"
#include "types.h"
#include "una.h"

/*** DMM structures ***/

typedef enum {
    // Driver errors.
    UNA_DMM_SUCCESS = 0,
    UNA_DMM_ERROR_NULL_PARAMETER,
    UNA_DMM_ERROR_SCAN_PERIOD_UNDERFLOW,
    UNA_DMM_ERROR_UPLINK_PERIOD_UNDERFLOW,
    UNA_DMM_ERROR_DOWNLINK_PERIOD_UNDERFLOW,
    // Low level drivers errors.s
    UNA_DMM_ERROR_BASE_NVM = 0x0100,
    // Last base value.
    UNA_DMM_ERROR_BASE_LAST = (UNA_DMM_ERROR_BASE_NVM + NVM_ERROR_BASE_LAST),
} UNA_DMM_status_t;

/*** DMM functions ***/

/*!******************************************************************
 * \fn UNA_DMM_status_t UNA_DMM_init(void)
 * \brief Init DMM registers.
 * \param[in]   none
 * \param[out]  none
 * \retval      Function execution status.
 *******************************************************************/
UNA_DMM_status_t UNA_DMM_init(void);

/*!******************************************************************
 * \fn UNA_DMM_status_t UNA_DMM_write_register(UNA_access_parameters_t* write_params, uint32_t reg_value, uint32_t reg_mask, UNA_access_status_t* write_status)
 * \brief Write DMM node register.
 * \param[in]   write_params: Pointer to the write operation parameters.
 * \param[in]   reg_value: Register value to write.
 * \param[in]   reg_mask: Writing operation mask.
 * \param[out]  write_status: Pointer to the writing operation status.
 * \retval      Function execution status.
 *******************************************************************/
UNA_DMM_status_t UNA_DMM_write_register(UNA_access_parameters_t* write_params, uint32_t reg_value, uint32_t reg_mask, UNA_access_status_t* write_status);

/*!******************************************************************
 * \fn UNA_DMM_status_t UNA_DMM_read_register(UNA_access_parameters_t* read_params, uint32_t* reg_value, UNA_access_status_t* read_status)
 * \brief Read DMM node register.
 * \param[in]   read_params: Pointer to the read operation parameters.
 * \param[out]  reg_value: Pointer to the read register value.
 * \param[out]  read_status: Pointer to the read operation status.
 * \retval      Function execution status.
 *******************************************************************/
UNA_DMM_status_t UNA_DMM_read_register(UNA_access_parameters_t* read_params, uint32_t* reg_value, UNA_access_status_t* read_status);

/*******************************************************************/
#define UNA_DMM_exit_error(base) { ERROR_check_exit(una_dmm_status, UNA_DMM_SUCCESS, base) }

/*******************************************************************/
#define UNA_DMM_stack_error(base) { ERROR_check_stack(una_dmm_status, UNA_DMM_SUCCESS, base) }

/*******************************************************************/
#define UNA_DMM_stack_exit_error(base, code) { ERROR_check_stack_exit(una_dmm_status, UNA_DMM_SUCCESS, base, code) }

#endif /* __UNA_DMM_H__ */
