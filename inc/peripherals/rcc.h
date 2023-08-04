/*
 * rcc.h
 *
 *  Created on: 25 oct. 2022
 *      Author: Ludo
 */

#ifndef __RCC_H__
#define __RCC_H__

#include "flash.h"
#include "types.h"

/*** RCC macros ***/

#define RCC_LSI_FREQUENCY_HZ	38000
#define RCC_LSE_FREQUENCY_HZ	32768
#define RCC_HSI_FREQUENCY_KHZ	16000

/*** RCC structures ***/

/*!******************************************************************
 * \enum RCC_status_t
 * \brief RCC driver error codes.
 *******************************************************************/
typedef enum {
	RCC_SUCCESS = 0,
	RCC_ERROR_NULL_PARAMETER,
	RCC_ERROR_HSI_READY,
	RCC_ERROR_HSI_SWITCH,
	RCC_ERROR_MSI_RANGE,
	RCC_ERROR_MSI_READY,
	RCC_ERROR_MSI_SWITCH,
	RCC_ERROR_BASE_FLASH = 0x0100,
	RCC_ERROR_BASE_LAST = (RCC_ERROR_BASE_FLASH + FLASH_ERROR_BASE_LAST)
} RCC_status_t;

/*!******************************************************************
 * \enum RCC_msi_range_t
 * \brief RCC MSI oscillator frequency ranges.
 *******************************************************************/
typedef enum {
	RCC_MSI_RANGE_0_65KHZ = 0,
	RCC_MSI_RANGE_1_131KHZ,
	RCC_MSI_RANGE_2_262KHZ,
	RCC_MSI_RANGE_3_524KKZ,
	RCC_MSI_RANGE_4_1MHZ,
	RCC_MSI_RANGE_5_2MHZ,
	RCC_MSI_RANGE_6_4MHZ,
	RCC_MSI_RANGE_LAST
} RCC_msi_range_t;

/*** RCC functions ***/

/*!******************************************************************
 * \fn void RCC_init(void)
 * \brief Init MCU default clock tree.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		none
 *******************************************************************/
void RCC_init(void);

/*!******************************************************************
 * \fn RCC_status_t RCC_switch_to_hsi(void)
 * \brief Switch system clock to 16MHz HSI.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		Function execution status.
 *******************************************************************/
RCC_status_t RCC_switch_to_hsi(void);

/*!******************************************************************
 * \fn RCC_status_t RCC_switch_to_msi(void)
 * \brief Switch system clock to MSI.
 * \param[in]  	msi_range: MSI frequency to set.
 * \param[out] 	none
 * \retval		Function execution status.
 *******************************************************************/
RCC_status_t RCC_switch_to_msi(RCC_msi_range_t msi_range);

/*!******************************************************************
 * \fn uint32_t RCC_get_sysclk_khz(void)
 * \brief Get the current system clock frequency.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		System clock frequency in kHz.
 *******************************************************************/
uint32_t RCC_get_sysclk_khz(void);

/*******************************************************************/
#define RCC_check_status(error_base) { if (rcc_status != RCC_SUCCESS) { status = error_base + rcc_status; goto errors; } }

/*******************************************************************/
#define RCC_stack_error(void) { ERROR_stack_error(rcc_status, RCC_SUCCESS, ERROR_BASE_RCC); }

/*******************************************************************/
#define RCC_print_error(void) { ERROR_print_error(rcc_status, RCC_SUCCESS, ERROR_BASE_RCC); }

#endif /* __RCC_H__ */
