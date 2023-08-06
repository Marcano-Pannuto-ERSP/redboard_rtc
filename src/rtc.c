// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023

#include <cli.h>
#include <am1815.h>

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

// disable unused pins (i.e., all pins except SPI and VBAT)
void disable_pins(struct am1815 *rtc){
    // disable EXBM, WDBM, RSEN, O4EN, O3EN, O1EN
    uint8_t read = am1815_read_register(rtc, 0x30);
    uint8_t mask = 0b11001111;
    uint8_t result = read & ~mask;
    am1815_write_register(rtc, 0x30, result);

    // disable O4BM
    uint8_t O4BM = am1815_read_register(rtc, 0x3F);
    uint8_t O4BMmask = 0b10000000;
    uint8_t O4BMresult = O4BM & ~O4BMmask;
    am1815_write_register(rtc, 0x3F, O4BMresult);

    // get access to BATMODE I/O register
    am1815_write_register(rtc, 0x1F, 0x9D);

    // disable SPI when we lose Vcc
    // set the BATMODE I/O register's 7th bit to 0
    // which means that the RTC will disable I/O interface in absence of vcc
    uint8_t IOBM = am1815_read_register(rtc, 0x27);
    uint8_t IOBMmask = 0b10000000;
    uint8_t IOBMresult = IOBM & ~IOBMmask;
    am1815_write_register(rtc, 0x27, IOBMresult);
}

// Set up registers that control the alarm
void configure_alarm(struct am1815 *rtc, bool enable, uint8_t pulse)
{
    // Configure AIRQ (alarm) interrupt
    // IM (level/pulse) AIE (enables interrupt) 0x12 intmask
    uint8_t alarm = am1815_read_register(rtc, 0x12);
    alarm = alarm & ~(0b01100100);

    // Enable/Disable the alarm
    if(enable){
        printf("alarm enabled\r\n");
    }
    else{
        printf("alarm disabled\r\n");
        am1815_write_register(rtc, 0x12, alarm);
        return;
    }

    // Set the alarm pulse
    if(pulse > 3){
        printf("ERROR: INVALID ARGUMENT\r\n");
        return;
    }
    printf("pulse given: %d\r\n", pulse);
    uint8_t alarmMask = pulse << 5;

    // enables the alarm
    alarmMask += 0b00000100;

    uint8_t alarmResult = alarm | alarmMask;
    am1815_write_register(rtc, 0x12, alarmResult);

    // Set Control2 register bits so that FOUT/nIRQ pin outputs nAIRQ
    uint8_t out = am1815_read_register(rtc, 0x11);
    uint8_t outMask = 0b00000011;
    uint8_t outResult = out | outMask;
    am1815_write_register(rtc, 0x11, outResult);

    // Set RPT bits in Countdown Timer Control register to control how often the alarm interrupt
    // repeats. Set it to 7 for now (once a second if hundredths alarm register contains 0)
    uint8_t timerControl = am1815_read_register(rtc, 0x18);
    uint8_t timerMask = 0b00011100;
    uint8_t timerResult = timerControl | timerMask;
    am1815_write_register(rtc, 0x18, timerResult);
}

// Set up registers that control the countdown timer
void configure_countdown(struct am1815 *rtc, double timer) {
    double period = am1815_write_timer(rtc, timer);
    
    if (period == 0) {
        // Disable the countdown timer
		uint8_t countdownTimer = am1815_read_register(rtc, 0x18);
		am1815_write_register(rtc, 0x18, countdownTimer & ~0b10000000);
        printf("Timer disabled (input is 0 or too close to 0).\r\n");
    } else {
        printf("Timer set to %f seconds.\r\n", period);
    }
}


