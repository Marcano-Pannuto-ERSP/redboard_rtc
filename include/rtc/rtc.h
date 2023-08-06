// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023

#ifndef RTC_H_
#define RTC_H_

#include <am1815.h>

// disable unused pins (i.e., all pins except SPI and VBAT)
void disable_pins(struct am1815 *rtc);

// Set up registers that control the alarm
void configure_alarm(struct am1815 *rtc, bool enable, uint8_t pulse);

// Set up registers that control the countdown timer
void configure_countdown(struct am1815 *rtc, double timer);

#endif//RTC_H_
