// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023

/** This is an example main executable program */

#include <example.h>

#include <uart.h>
#include <syscalls.h>

#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"

#include <sys/time.h>

#include <string.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

struct uart uart;
struct am1815 rtc;
struct spi_bus spi;
struct spi_device rtc_spi;

__attribute__((constructor))
static void redboard_init(void)
{
	// Prepare MCU by init-ing clock, cache, and power level operation
	am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_SYSCLK_MAX, 0);
	am_hal_cachectrl_config(&am_hal_cachectrl_defaults);
	am_hal_cachectrl_enable();
	am_bsp_low_power_init();
	am_hal_sysctrl_fpu_enable();
	am_hal_sysctrl_fpu_stacking_enable(true);

	uart_init(&uart, UART_INST0);
	syscalls_uart_init(&uart);

	// After init is done, enable interrupts
	am_hal_interrupt_master_enable();
}

__attribute__((destructor))
static void redboard_shutdown(void)
{
	// Any destructors/code that should run when main returns should go here
}

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
void configure_alarm(struct am1815 *rtc){
    // Configure AIRQ (alarm) interrupt
    // IM (level/pulse) AIE (enables interrupt) 0x12 intmask
    uint8_t alarm = am1815_read_register(rtc, 0x12);
    alarm = alarm & ~(0b01100100);

	// SHOULD NORMALLY GIVE A PARAMETER FOR PULSE
	// HARDCODING FOR NOW FOR TESTING
    // uint8_t alarmMask = int(pulse) << 5;
	// uint8_t alarmMask = (int)(1/8192) << 5;
    uint8_t alarmMask = 1 << 5;

	// DON'T RUN NEXT LINE IF WANT TO DISABLE ALARM
	// HARDCODING FOR NOW FOR TESTING
	// if not d:
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

int main(void)
{
	am_util_stdio_printf("Hello World!!!\r\n");
	printf("Hello World!!!\r\n");

	spi_bus_init(&spi, 0);
	spi_bus_enable(&spi);
	spi_bus_init_device(&spi, &rtc_spi, SPI_CS_3, 2000000u);
	am1815_init(&rtc, &rtc_spi);

	// Initialize the pins
	
	// SHOULD BE ABLE TO ENABLE OR DISABLE TRICKLE
	// HARDCODING FOR NOW FOR TESTING (DISABLE)
	am1815_disable_trickle(&rtc);
    // printf("checkpoint: disable trickle\r\n");

	// disable unused pins
	disable_pins(&rtc);
    // printf("checkpoint: disable pins\r\n");

	// get access to osillator control register
	am1815_write_register(&rtc, 0x1F, 0xA1);

	// clear the OF bit so that a failure isn't detected on start up
    uint8_t OF = am1815_read_register(&rtc, 0x1D);
    uint8_t OFmask = 0b00000010;
    uint8_t OFresult = OF & ~OFmask;
    am1815_write_register(&rtc, 0x1D, OFresult);
    // printf("checkpoint: failure not detected\r\n");

    // Enable or disable automatic switch over from the crystal to the internal RC clock
    // Default FOS to 1, AOS to 0, and change them if user used the flags

	// HARDCODING FOR NOW FOR TESTING (DEFAULT VALUES)
    uint8_t osCtrl = am1815_read_register(&rtc, 0x1C);
    uint8_t FOSmask = 0b00001000;
    // if f:
    //     # set FOS to 0
    //     FOSresult = osCtrl & ~FOSmask
    // else:
	// set FOS to 1 (default);
	uint8_t FOSresult = osCtrl | FOSmask;
	am1815_write_register(&rtc, 0x1C, FOSresult);

    osCtrl = am1815_read_register(&rtc, 0x1C);

    // get access to oscillator control register
    am1815_write_register(&rtc, 0x1F, 0xA1);

    uint8_t AOSmask = 0b00010000;
    // if a:
    //     # set AOS to 1
    //     AOSresult = osCtrl | AOSmask
    // else:
    //     # set AOS to 0 (default)
    uint8_t AOSresult = osCtrl & ~AOSmask;
    am1815_write_register(&rtc, 0x1C, AOSresult);
    // printf("checkpoint: automatic switching\r\n");

	// Configure alarm
	configure_alarm(&rtc);
    // printf("checkpoint: configure alarm\r\n");

    // Write to bit 7 of register 1 to signal that this program initialized the RTC
    uint8_t sec = am1815_read_register(&rtc, 0x01);
    uint8_t secMask = 0b10000000;
    uint8_t secResult = sec | secMask;
    am1815_write_register(&rtc, 0x01, secResult);

	printf("done!\r\n");

    // am1815_write_register(&rtc, 0x1F, 0x3C);
    // printf("SOFT RESET\r\n");

	return 0;
}
