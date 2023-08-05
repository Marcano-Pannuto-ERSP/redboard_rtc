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
#include <cli.h>

struct uart uart;
struct am1815 rtc;
struct spi_bus spi;
struct spi_device rtc_spi;
struct cli cli;

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
    
    // Enable/Disable the alarm
    printf("Enter 1 for enabling the alarm\r\n");
    cli_line_buffer *alarmbuf;
    alarmbuf = cli_read_line(&cli);
    int alarmable;
    sscanf(alarmbuf, "%d", &alarmable);
    if(alarmable == 1){
        printf("alarm enabled\r\n");
    }
    else{
        printf("alarm disabled\r\n");
        am1815_write_register(rtc, 0x12, alarm);
        return;
    }

    // Set the alarm pulse
    printf("Enter one of the choices for pulse: {0,1,2,3} (1 means 1/8192 seconds for XT and 1/64 sec for RC, 2 means 1/64 s for both, 3 means 1/4 s for both)\r\n");
    cli_line_buffer *pulsebuf;
    pulsebuf = cli_read_line(&cli);
    int pulse;
    sscanf(pulsebuf, "%d", &pulse);
    if(pulse < 0 || pulse > 3){
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

int main(void)
{
	am_util_stdio_printf("Hello World!!!\r\n");
	printf("Hello World!!!\r\n");

	spi_bus_init(&spi, 0);
	spi_bus_enable(&spi);
	spi_bus_init_device(&spi, &rtc_spi, SPI_CS_3, 2000000u);
	am1815_init(&rtc, &rtc_spi);
    cli_initialize(&cli);

    // Enable/Disable trickle charging
    printf("Enter 1 for enabling trickle charge\r\n");
    cli_line_buffer *tricklebuf;
    tricklebuf = cli_read_line(&cli);
    int trickle;
    sscanf(tricklebuf, "%d", &trickle);
    if(trickle == 1){
        am1815_enable_trickle(&rtc);
        printf("trickle charge enabled\r\n");
    }
    else{
        am1815_disable_trickle(&rtc);
        printf("trickle charge disabled\r\n");
    }

	// disable unused pins
	disable_pins(&rtc);

	// get access to osillator control register
	am1815_write_register(&rtc, 0x1F, 0xA1);

	// clear the OF bit so that a failure isn't detected on start up
    uint8_t OF = am1815_read_register(&rtc, 0x1D);
    uint8_t OFmask = 0b00000010;
    uint8_t OFresult = OF & ~OFmask;
    am1815_write_register(&rtc, 0x1D, OFresult);

    // Enable or disable automatic switch over from the crystal to the internal RC clock
    // Default FOS to 1, AOS to 0, and change them if user used the flags
    printf("Enter 0 to disable automatic switching when an oscillator failure is detected\r\n");
    cli_line_buffer *FOSbuf;
    FOSbuf = cli_read_line(&cli);
    int FOS;
    sscanf(FOSbuf, "%d", &FOS);
    uint8_t osCtrl = am1815_read_register(&rtc, 0x1C);
    uint8_t FOSmask = 0b00001000;
    uint8_t FOSresult;
    if(FOS == 0){
        // set FOS to 0
        FOSresult = osCtrl & ~FOSmask;
        printf("disabled automatic switching when an oscillator failure is detected\r\n");
    }
    else{
        // set FOs to 1 (default)
        FOSresult = osCtrl | FOSmask;
        printf("enabled automatic switching when an oscillator failure is detected\r\n");
    }
	am1815_write_register(&rtc, 0x1C, FOSresult);

    osCtrl = am1815_read_register(&rtc, 0x1C);

    // get access to oscillator control register
    am1815_write_register(&rtc, 0x1F, 0xA1);

    // HARDCODING
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

    // unsigned char data[4] = {0};
    // size_t size = 4;
    // size_t read = 0;
    // while((read = uart_read(&uart, data, size)) == 0);
    // while(data != '\r'){
    //     while((read = uart_read(&uart, data, 1)) == 0);
    //     uart_read(&uart, data, size);
    // }
    // printf("bytes read: %d\r\n", (int)read);
    // printf("data read1: %s\r\n", data);

    // unsigned char* a, b;
    // printf("first: ");
    // scanf("%s, &a");

    // am1815_write_register(&rtc, 0x1F, 0x3C);
    // printf("SOFT RESET\r\n");

    printf("done!\r\n");

	return 0;
}
