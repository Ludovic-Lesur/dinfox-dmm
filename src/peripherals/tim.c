/*
 * tim.c
 *
 *  Created on: 22 aug. 2020
 *      Author: Ludo
 */

#include "tim.h"

#include "iwdg.h"
#include "nvic.h"
#include "pwr.h"
#include "rcc.h"
#include "rcc_reg.h"
#include "rtc.h"
#include "tim_reg.h"
#include "types.h"

/*** TIM local macros ***/

#define TIM_TIMEOUT_COUNT				10000000

#define TIM2_CNT_VALUE_MAX				0xFFFF

#define TIM2_TARGET_TRIGGER_CLOCK_HZ	2048

#define TIM2_PRESCALER_ETRF_LSE			1
#define TIM2_PRESCALER_PSC_LSE			((RCC_LSE_FREQUENCY_HZ) / (TIM2_TARGET_TRIGGER_CLOCK_HZ * TIM2_PRESCALER_ETRF_LSE))

#define TIM2_CLOCK_SWITCH_LATENCY_MS	2

#define TIM2_TIMER_DURATION_MS_MIN		1
#define TIM2_TIMER_DURATION_MS_MAX		((TIM2_CNT_VALUE_MAX * 1000) / (tim2_ctx.etrf_clock_hz))

#define TIM2_WATCHDOG_PERIOD_SECONDS	((TIM2_TIMER_DURATION_MS_MAX / 1000) + 5)

#define TIM3_NUMBER_OF_CHANNELS			4
#define TIM3_NUMBER_OF_USED_CHANNELS	3
#define TIM3_CCRX_MASK_OFF				0xFFFF
#define TIM3_PWM_FREQUENCY_HZ			10000

#define TIM21_INPUT_CAPTURE_PRESCALER	8

#define TIM22_PRESCALER					8
#define TIM22_DIMMING_LUT_LENGTH		100

/*** TIM local structures ***/

/*******************************************************************/
typedef struct {
	uint32_t duration_ms;
	TIM_waiting_mode_t waiting_mode;
	volatile uint8_t running_flag;
	volatile uint8_t irq_flag;
} TIM_channel_context_t;

/*******************************************************************/
typedef struct {
	uint32_t etrf_clock_hz;
	TIM_channel_context_t channel[TIM2_CHANNEL_LAST];
} TIM2_context_t;

/*******************************************************************/
typedef struct {
	volatile uint16_t ccr1_start;
	volatile uint16_t ccr1_end;
	volatile uint16_t capture_count;
	volatile uint8_t capture_done;
} TIM21_context_t;

/*******************************************************************/
typedef struct {
	volatile uint32_t dimming_lut_idx;
	volatile uint8_t dimming_lut_direction;
	volatile uint8_t single_blink_done;
} TIM22_context_t;

/*** TIM local global variables ***/

static TIM2_context_t tim2_ctx;

static const uint8_t TIM3_LED_CHANNELS[TIM3_NUMBER_OF_USED_CHANNELS] = {
	TIM3_CHANNEL_LED_RED,
	TIM3_CHANNEL_LED_GREEN,
	TIM3_CHANNEL_LED_BLUE
};
static uint16_t tim3_ccrx_mask[TIM3_NUMBER_OF_CHANNELS];

static TIM21_context_t tim21_ctx;

static const uint16_t TIM22_DIMMING_LUT[TIM22_DIMMING_LUT_LENGTH] = {
	1601, 1601, 1601, 1601, 1601, 1601, 1600, 1600, 1600, 1600,
	1600, 1600, 1600, 1599, 1599, 1599, 1599, 1598, 1598, 1598,
	1598, 1597, 1597, 1596, 1596, 1596, 1595, 1595, 1594, 1593,
	1593, 1592, 1591, 1590, 1589, 1588, 1587, 1586, 1585, 1584,
	1582, 1581, 1579, 1577, 1575, 1573, 1571, 1569, 1566, 1563,
	1560, 1557, 1554, 1550, 1546, 1542, 1537, 1532, 1527, 1521,
	1514, 1508, 1500, 1493, 1484, 1475, 1465, 1454, 1443, 1431,
	1418, 1403, 1388, 1371, 1353, 1334, 1314, 1291, 1267, 1241,
	1213, 1183, 1151, 1116, 1078, 1038, 994,  947,  896,  842,
	783,  720,  651,  578,  498,  413,  321,  222,  115,  0,
};
static TIM22_context_t tim22_ctx;

/*** TIM local functions ***/

/*******************************************************************/
void __attribute__((optimize("-O0"))) TIM2_IRQHandler(void) {
	// Local variables.
	uint8_t channel_idx = 0;
	uint8_t channel_mask = 0;
	// Channels loop.
	for (channel_idx=0 ; channel_idx<TIM2_CHANNEL_LAST ; channel_idx++) {
		// Compute mask.
		channel_mask = (0b1 << (channel_idx + 1));
		// Check flag.
		if (((TIM2 -> SR) & channel_mask) != 0) {
			// Set local flag if channel is active.
			tim2_ctx.channel[channel_idx].irq_flag = tim2_ctx.channel[channel_idx].running_flag;
			// Clear flag.
			TIM2 -> SR &= ~(channel_mask);
		}
	}
}

/*******************************************************************/
void __attribute__((optimize("-O0"))) TIM21_IRQHandler(void) {
	// TI1 interrupt.
	if (((TIM21 -> SR) & (0b1 << 1)) != 0) {
		// Update flags.
		if (((TIM21 -> DIER) & (0b1 << 1)) != 0) {
			// Check count.
			if (tim21_ctx.capture_count == 0) {
				// Store start value.
				tim21_ctx.ccr1_start = (TIM21 -> CCR1);
				tim21_ctx.capture_count++;
			}
			else {
				// Check rollover.
				if ((TIM21 -> CCR1) > tim21_ctx.ccr1_end) {
					// Store new value.
					tim21_ctx.ccr1_end = (TIM21 -> CCR1);
					tim21_ctx.capture_count++;
				}
				else {
					// Capture complete.
					tim21_ctx.capture_done = 1;
				}
			}
		}
		TIM21 -> SR &= ~(0b1 << 1);
	}
}

/*******************************************************************/
void __attribute__((optimize("-O0"))) TIM22_IRQHandler(void) {
	// Local variables.
	uint8_t idx = 0;
	// Check update flag.
	if (((TIM22 -> SR) & (0b1 << 0)) != 0) {
		// Update duty cycles.
		for (idx=0 ; idx<TIM3_NUMBER_OF_USED_CHANNELS ; idx++) {
			TIM3 -> CCRx[TIM3_LED_CHANNELS[idx]] = (TIM22_DIMMING_LUT[tim22_ctx.dimming_lut_idx] | tim3_ccrx_mask[TIM3_LED_CHANNELS[idx]]);
		}
		// Manage index and direction.
		if (tim22_ctx.dimming_lut_direction == 0) {
			// Increment index.
			tim22_ctx.dimming_lut_idx++;
			// Invert direction at end of table.
			if (tim22_ctx.dimming_lut_idx >= (TIM22_DIMMING_LUT_LENGTH - 1)) {
				tim22_ctx.dimming_lut_direction = 1;
			}
		}
		else {
			// Decrement index.
			tim22_ctx.dimming_lut_idx--;
			// Invert direction at the beginning of table.
			if (tim22_ctx.dimming_lut_idx == 0) {
				// Single blink done.
				TIM3_stop();
				TIM22_stop();
				tim22_ctx.dimming_lut_direction = 0;
				tim22_ctx.single_blink_done = 1;
			}
		}
		// Clear flag.
		TIM22 -> SR &= ~(0b1 << 0);
	}
}

/*******************************************************************/
static void _TIM2_compute_compare_value(TIM2_channel_t channel) {
	// Update compare value.
	TIM2 -> CCRx[channel] = ((TIM2 -> CNT) + ((tim2_ctx.channel[channel].duration_ms * tim2_ctx.etrf_clock_hz) / (1000))) % TIM2_CNT_VALUE_MAX;
}

/*******************************************************************/
static TIM_status_t _TIM2_internal_watchdog(uint32_t time_start, uint32_t* time_reference) {
	// Local variables.
	TIM_status_t status = TIM_SUCCESS;
	uint32_t time = RTC_get_time_seconds();
	// If the RTC is correctly clocked, it will be used as internal watchdog and the IWDG can be reloaded.
	// If the RTC is not running anymore due to a clock failure, the IWDG is not reloaded and will reset the MCU.
	if (time != (*time_reference)) {
		// Update time reference and reload IWDG.
		(*time_reference) = time;
		IWDG_reload();
	}
	// Internal watchdog.
	if (time > (time_start + TIM2_WATCHDOG_PERIOD_SECONDS)) {
		status = TIM_ERROR_COMPLETION_WATCHDOG;
	}
	return status;
}

/*******************************************************************/
void _TIM3_reset_channels(void) {
	// Local variables.
	uint8_t idx = 0;
	// Channels loop.
	for (idx=0 ; idx<TIM3_NUMBER_OF_CHANNELS ; idx++) {
		// Reset mask.
		tim3_ccrx_mask[idx] = TIM3_CCRX_MASK_OFF;
		// Disable channel.
		TIM3 -> CCRx[idx] = (TIM3 -> ARR) + 1;
	}
	// Reset counter.
	TIM3 -> CNT = 0;
}

/*** TIM functions ***/

/*******************************************************************/
void TIM2_init(void) {
	// Local variables.
	uint8_t channel_idx = 0;
	// Init context.
	for (channel_idx=0 ; channel_idx<TIM2_CHANNEL_LAST ; channel_idx++) {
		tim2_ctx.channel[channel_idx].duration_ms = 0;
		tim2_ctx.channel[channel_idx].waiting_mode = TIM_WAITING_MODE_ACTIVE;
		tim2_ctx.channel[channel_idx].running_flag = 0;
		tim2_ctx.channel[channel_idx].irq_flag = 0;
	}
	// Enable peripheral clock.
	RCC -> APB1ENR |= (0b1 << 0); // TIM2EN='1'.
	RCC -> APB1SMENR |= (0b1 << 0); // TIM2SMEN='1'.
	// Use LSE as trigger.
	RCC -> CR &= ~(0b1 << 5); // HSI16OUTEN='0'.
	TIM2 -> SMCR &= ~(0b11 << 12); // No prescaler on ETRF.
	TIM2 -> PSC = (TIM2_PRESCALER_PSC_LSE - 1);
	TIM2 -> OR |= (0b101 << 0);
	// Update context.
	tim2_ctx.etrf_clock_hz = ((RCC_LSE_FREQUENCY_HZ) / (TIM2_PRESCALER_ETRF_LSE * TIM2_PRESCALER_PSC_LSE));
	// No overflow.
	TIM2 -> ARR |= 0x0000FFFF;
	// Use external clock mode 2.
	TIM2 -> SMCR |= (0b1 << 14) | (0b111 << 4);
	// Configure channels 1-4 in output compare mode.
	TIM2 -> CCMR1 &= 0xFFFF0000;
	TIM2 -> CCMR2 &= 0xFFFF0000;
	TIM2 -> CCER &= 0xFFFF0000;
	// Generate event to update registers.
	TIM2 -> EGR |= (0b1 << 0); // UG='1'.
	// Enable interrupt.
	NVIC_enable_interrupt(NVIC_INTERRUPT_TIM2, NVIC_PRIORITY_TIM2);
}

/*******************************************************************/
void TIM2_de_init(void) {
	// Disable interrupt.
	NVIC_disable_interrupt(NVIC_INTERRUPT_TIM2);
	// Disable peripheral clock.
	RCC -> APB1ENR &= ~(0b1 << 0); // TIM2EN='0'.
}

/*******************************************************************/
TIM_status_t TIM2_start(TIM2_channel_t channel, uint32_t duration_ms, TIM_waiting_mode_t waiting_mode) {
	// Local variables.
	TIM_status_t status = TIM_SUCCESS;
	uint32_t local_duration_ms = duration_ms;
	uint32_t duration_min_ms = TIM2_TIMER_DURATION_MS_MIN;
	// Check parameters.
	if (waiting_mode >= TIM_WAITING_MODE_LAST) {
		status = TIM_ERROR_WAITING_MODE;
		goto errors;
	}
	// Check waiting mode.
	if (waiting_mode == TIM_WAITING_MODE_LOW_POWER_SLEEP) {
		// Compensate clock switch latency.
		duration_min_ms += TIM2_CLOCK_SWITCH_LATENCY_MS;
	}
	// Check parameters.
	if (channel >= TIM2_CHANNEL_LAST) {
		status = TIM_ERROR_CHANNEL;
		goto errors;
	}
	if (duration_ms < duration_min_ms) {
		status = TIM_ERROR_DURATION_UNDERFLOW;
		goto errors;
	}
	if (duration_ms > TIM2_TIMER_DURATION_MS_MAX) {
		status = TIM_ERROR_DURATION_OVERFLOW;
		goto errors;
	}
	if (waiting_mode == TIM_WAITING_MODE_LOW_POWER_SLEEP) {
		local_duration_ms -= TIM2_CLOCK_SWITCH_LATENCY_MS;
	}
	// Update channel context.
	tim2_ctx.channel[channel].duration_ms = local_duration_ms;
	tim2_ctx.channel[channel].waiting_mode = waiting_mode;
	tim2_ctx.channel[channel].running_flag = 1;
	tim2_ctx.channel[channel].irq_flag = 0;
	// Compute compare value.
	_TIM2_compute_compare_value(channel);
	// Clear flag.
	TIM2 -> SR &= ~(0b1 << (channel + 1));
	// Enable channel.
	TIM2 -> DIER |= (0b1 << (channel + 1));
	// Enable counter.
	TIM2 -> CR1 |= (0b1 << 0);
errors:
	return status;
}

/*******************************************************************/
TIM_status_t TIM2_stop(TIM2_channel_t channel) {
	// Local variables.
	TIM_status_t status = TIM_SUCCESS;
	// Check parameter.
	if (channel >= TIM2_CHANNEL_LAST) {
		status = TIM_ERROR_CHANNEL;
		goto errors;
	}
	// Disable interrupt.
	TIM2 -> DIER &= ~(0b1 << (channel + 1));
	// Clear flag.
	TIM2 -> SR &= ~(0b1 << (channel + 1));
	// Disable channel.
	tim2_ctx.channel[channel].running_flag = 0;
	tim2_ctx.channel[channel].irq_flag = 0;
	// Disable counter if all channels are stopped.
	if (((TIM2 -> DIER) & 0x0000001E) == 0) {
		TIM2 -> CR1 &= ~(0b1 << 0);
	}
errors:
	return status;
}

/*******************************************************************/
TIM_status_t TIM2_get_status(TIM2_channel_t channel, uint8_t* timer_has_elapsed) {
	// Local variables.
	TIM_status_t status = TIM_SUCCESS;
	// Check parameters.
	if (channel >= TIM2_CHANNEL_LAST) {
		status = TIM_ERROR_CHANNEL;
		goto errors;
	}
	if (timer_has_elapsed == NULL) {
		status = TIM_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Update flag.
	(*timer_has_elapsed) = ((tim2_ctx.channel[channel].running_flag == 0) || (tim2_ctx.channel[channel].irq_flag != 0)) ? 1 : 0;
errors:
	return status;
}

/*******************************************************************/
TIM_status_t TIM2_wait_completion(TIM2_channel_t channel) {
	// Local variables.
	TIM_status_t status = TIM_SUCCESS;
	RCC_status_t rcc_status = RCC_SUCCESS;
	uint32_t time_start = RTC_get_time_seconds();
	uint32_t time_reference = 0;
	// Check parameters.
	if (channel >= TIM2_CHANNEL_LAST) {
		status = TIM_ERROR_CHANNEL;
		goto errors;
	}
	// Directly exit if the IRQ already occurred.
	if ((tim2_ctx.channel[channel].running_flag == 0) || (tim2_ctx.channel[channel].irq_flag != 0)) goto errors;
	// Sleep until channel is not running.
	switch (tim2_ctx.channel[channel].waiting_mode) {
	case TIM_WAITING_MODE_ACTIVE:
		// Active loop.
		while (tim2_ctx.channel[channel].irq_flag == 0) {
			// Internal watchdog.
			status = _TIM2_internal_watchdog(time_start, &time_reference);
			if (status != TIM_SUCCESS) goto errors;
		}
		break;
	case TIM_WAITING_MODE_SLEEP:
		// Enter sleep mode.
		while (tim2_ctx.channel[channel].irq_flag == 0) {
			PWR_enter_sleep_mode();
			// Internal watchdog.
			status = _TIM2_internal_watchdog(time_start, &time_reference);
			if (status != TIM_SUCCESS) goto errors;
		}
		break;
	case TIM_WAITING_MODE_LOW_POWER_SLEEP:
		// Switch to MSI.
		rcc_status = RCC_switch_to_msi(RCC_MSI_RANGE_1_131KHZ);
		RCC_exit_error(TIM_ERROR_BASE_RCC);
		// Enter low power sleep mode.
		while (tim2_ctx.channel[channel].irq_flag == 0) {
			PWR_enter_low_power_sleep_mode();
			// Internal watchdog.
			status = _TIM2_internal_watchdog(time_start, &time_reference);
			if (status != TIM_SUCCESS) goto errors;
		}
		// Go back to HSI.
		rcc_status = RCC_switch_to_hsi();
		RCC_exit_error(TIM_ERROR_BASE_RCC);
		break;
	default:
		status = TIM_ERROR_WAITING_MODE;
		goto errors;
	}
errors:
	// Clear flag and update compare value for next IRQ.
	tim2_ctx.channel[channel].irq_flag = 0;
	_TIM2_compute_compare_value(channel);
	return status;
}

/*******************************************************************/
TIM_status_t TIM3_init(void) {
	// Local variables.
	TIM_status_t status = TIM_SUCCESS;
	RCC_status_t rcc_status = RCC_SUCCESS;
	uint32_t tim3_clock_hz = 0;
	uint8_t idx = 0;
	// Get clock source frequency.
	rcc_status = RCC_get_frequency_hz(RCC_CLOCK_HSI, &tim3_clock_hz);
	RCC_exit_error(TIM_ERROR_BASE_RCC);
	// Enable peripheral clock.
	RCC -> APB1ENR |= (0b1 << 1); // TIM3EN='1'.
	// Set PWM frequency.
	TIM3 -> ARR = (tim3_clock_hz / TIM3_PWM_FREQUENCY_HZ);
	// Configure channels 1-4 in PWM mode 1 (OCxM='110' and OCxPE='1').
	TIM3 -> CCMR1 |= (0b110 << 12) | (0b1 << 11) | (0b110 << 4) | (0b1 << 3);
	TIM3 -> CCMR2 |= (0b110 << 12) | (0b1 << 11) | (0b110 << 4) | (0b1 << 3);
	TIM3 -> CR1 |= (0b1 << 7);
	// Enable required channels.
	for (idx=0 ; idx<TIM3_NUMBER_OF_USED_CHANNELS ; idx++) {
		TIM3 -> CCER |= (0b1 << (TIM3_LED_CHANNELS[idx] << 2));
	}
	// Disable all channels by default.
	_TIM3_reset_channels();
	// Generate event to update registers.
	TIM3 -> EGR |= (0b1 << 0); // UG='1'.
errors:
	return status;
}

/*******************************************************************/
void TIM3_start(TIM3_channel_mask_t led_color) {
	// Local variables.
	uint8_t idx = 0;
	// Disable all channels.
	_TIM3_reset_channels();
	// Enable required channels.
	for (idx=0 ; idx<TIM3_NUMBER_OF_CHANNELS ; idx++) {
		if ((led_color & (0b1 << idx)) != 0) {
			tim3_ccrx_mask[idx] = 0;
		}
	}
	// Enable counter.
	TIM3 -> CR1 |= (0b1 << 0); // CEN='1'.
}

/*******************************************************************/
void TIM3_stop(void) {
	// Disable all channels.
	_TIM3_reset_channels();
	// Disable and reset counter.
	TIM3 -> CR1 &= ~(0b1 << 0); // CEN='0'.
}

/*******************************************************************/
void TIM21_init(void) {
	// Enable peripheral clock.
	RCC -> APB2ENR |= (0b1 << 2); // TIM21EN='1'.
	// Configure timer.
	// Channel input on TI1.
	// Capture done every 8 edges.
	// CH1 mapped on MCO.
	TIM21 -> CCMR1 |= (0b01 << 0) | (0b11 << 2);
	TIM21 -> OR |= (0b111 << 2);
	// Enable interrupt.
	TIM21 -> DIER |= (0b1 << 1); // CC1IE='1'.
	// Generate event to update registers.
	TIM21 -> EGR |= (0b1 << 0); // UG='1'.
}

/*******************************************************************/
void TIM21_de_init(void) {
	// Disable timer.
	TIM21 -> CR1 &= ~(0b1 << 0); // CEN='0'.
	// Disable peripheral clock.
	RCC -> APB2ENR &= ~(0b1 << 2); // TIM21EN='0'.
}

/*******************************************************************/
TIM_status_t TIM21_mco_capture(uint16_t* ref_clock_pulse_count, uint16_t* mco_pulse_count) {
	// Local variables.
	TIM_status_t status = TIM_SUCCESS;
	uint32_t loop_count = 0;
	// Check parameters.
	if ((ref_clock_pulse_count == NULL) || (mco_pulse_count == NULL)) {
		status = TIM_ERROR_NULL_PARAMETER;
		goto errors;
	}
	// Reset timer context.
	tim21_ctx.ccr1_start = 0;
	tim21_ctx.ccr1_end = 0;
	tim21_ctx.capture_count = 0;
	tim21_ctx.capture_done = 0;
	// Reset counter.
	TIM21 -> CNT = 0;
	TIM21 -> CCR1 = 0;
	// Enable interrupt.
	TIM21 -> SR &= 0xFFFFF9B8; // Clear all flags.
	NVIC_enable_interrupt(NVIC_INTERRUPT_TIM21, NVIC_PRIORITY_TIM21);
	// Enable TIM21 peripheral.
	TIM21 -> CR1 |= (0b1 << 0); // CEN='1'.
	TIM21 -> CCER |= (0b1 << 0); // CC1E='1'.
	// Wait for capture to complete.
	while (tim21_ctx.capture_done == 0) {
		// Manage timeout.
		loop_count++;
		if (loop_count > TIM_TIMEOUT_COUNT) {
			status = TIM_ERROR_CAPTURE_TIMEOUT;
			goto errors;
		}
	}
	// Update results.
	(*ref_clock_pulse_count) = (tim21_ctx.ccr1_end - tim21_ctx.ccr1_start);
	(*mco_pulse_count) = (TIM21_INPUT_CAPTURE_PRESCALER * (tim21_ctx.capture_count - 1));
errors:
	// Disable interrupt.
	NVIC_disable_interrupt(NVIC_INTERRUPT_TIM21);
	// Stop counter.
	TIM21 -> CR1 &= ~(0b1 << 0); // CEN='0'.
	TIM21 -> CCER &= ~(0b1 << 0); // CC1E='0'.
	return status;
}

/*******************************************************************/
void TIM22_init(void) {
	// Init context.
	tim22_ctx.dimming_lut_idx = 0;
	tim22_ctx.dimming_lut_direction = 0;
	tim22_ctx.single_blink_done = 1;
	// Enable peripheral clock.
	RCC -> APB2ENR |= (0b1 << 2); // TIM22EN='1'.
	// Configure period.
	TIM22 -> PSC = (TIM22_PRESCALER - 1); // Timer is clocked on (SYSCLK / 8).
	// Generate event to update registers.
	TIM22 -> EGR |= (0b1 << 0); // UG='1'.
	// Enable interrupt.
	TIM22 -> DIER |= (0b1 << 0);
}

/*******************************************************************/
TIM_status_t TIM22_start(uint32_t led_blink_period_ms) {
	// Local variables.
	TIM_status_t status = TIM_SUCCESS;
	RCC_status_t rcc_status = RCC_SUCCESS;
	uint32_t tim22_clock_hz = 0;
	uint64_t arr = 0;
	// Get clock source frequency.
	rcc_status = RCC_get_frequency_hz(RCC_CLOCK_HSI, &tim22_clock_hz);
	RCC_exit_error(TIM_ERROR_BASE_RCC);
	// Reset LUT index and flag.
	tim22_ctx.dimming_lut_idx = 0;
	tim22_ctx.dimming_lut_direction = 0;
	tim22_ctx.single_blink_done = 0;
	// Set period.
	TIM22 -> CNT = 0;
	arr = ((uint64_t) led_blink_period_ms) * ((uint64_t) tim22_clock_hz);
	arr /= (1000 * TIM22_PRESCALER * 2 * TIM22_DIMMING_LUT_LENGTH);
	TIM22 -> ARR = (uint32_t) arr;
	// Clear flag and enable interrupt.
	TIM22 -> SR &= ~(0b1 << 0); // Clear flag (UIF='0').
	NVIC_enable_interrupt(NVIC_INTERRUPT_TIM22, NVIC_PRIORITY_TIM22);
	// Enable TIM22 peripheral.
	TIM22 -> CR1 |= (0b1 << 0); // Enable TIM22 (CEN='1').
errors:
	return status;
}

/*******************************************************************/
void TIM22_stop(void) {
	// Disable interrupt.
	NVIC_disable_interrupt(NVIC_INTERRUPT_TIM22);
	// Stop TIM22.
	TIM22 -> CR1 &= ~(0b1 << 0); // CEN='0'.
}

/*******************************************************************/
uint8_t TIM22_is_single_blink_done(void) {
	return (tim22_ctx.single_blink_done);
}
