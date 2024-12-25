/*
 * una_at_hw.c
 *
 *  Created on: 23 dec. 2024
 *      Author: Ludo
 */

#include "una_at_hw.h"

#ifndef UNA_AT_DISABLE_FLAGS_FILE
#include "una_at_flags.h"
#endif
#include "iwdg.h"
#include "lptim.h"
#include "una_at.h"
#include "types.h"

#ifndef UNA_AT_DISABLE

/*** UNA AT HW functions ***/

/*******************************************************************/
UNA_AT_status_t UNA_AT_HW_delay_milliseconds(uint32_t delay_ms) {
    // Local variables.
    UNA_AT_status_t status = UNA_AT_SUCCESS;
    LPTIM_status_t lptim_status = LPTIM_SUCCESS;
    // Reload watchdog.
    IWDG_reload();
    // Perform delay.
    lptim_status = LPTIM_delay_milliseconds(delay_ms, LPTIM_DELAY_MODE_SLEEP);
    LPTIM_exit_error(UNA_AT_ERROR_BASE_DELAY);
errors:
    return status;
}

#endif /* UNA_AT_DISABLE */

