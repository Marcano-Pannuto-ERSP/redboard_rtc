// test time synchronization

#include <rtc.h>

#include <cli.h>
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
#include <inttypes.h>

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

	spi_bus_init(&spi, 0);
	spi_bus_enable(&spi);
	spi_bus_init_device(&spi, &rtc_spi, SPI_CS_3, 2000000u);
	am1815_init(&rtc, &rtc_spi);

	// cli_init(&cli);
	uart_init(&uart, UART_INST0);
	syscalls_uart_init(&uart);
	syscalls_rtc_init(&rtc);

	// After init is done, enable interrupts
	am_hal_interrupt_master_enable();
}

__attribute__((destructor))
static void redboard_shutdown(void)
{
	// cli_destroy(&cli);
}

int main(void)
{
    // test am1815_write_time
    struct timeval curr_time = am1815_read_time(&rtc);
    am_util_stdio_printf("Time before change: %llu seconds, %ld microseconds\r\n", curr_time.tv_sec, curr_time.tv_usec);

    struct timeval new_time = {.tv_sec = 1000003000, .tv_usec = 40};
    am1815_write_time(&rtc, &new_time);
    curr_time = am1815_read_time(&rtc);
    am_util_stdio_printf("Time after change: %llu seconds, %ld microseconds\r\n", curr_time.tv_sec, curr_time.tv_usec);

    // run test-time-server.py
    am_util_stdio_printf("Begin delay...\r\n");
    am_util_delay_ms(3000);
    am_util_stdio_printf("...End delay\r\n");

    int buffer_size = 50;
    unsigned char buffer[buffer_size];
    uart_read(&uart, buffer, buffer_size);
    am_util_stdio_printf("Received from serial: %s\r\n", buffer);
    long seconds;
    // strtol(&buffer, &seconds);
}